#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/json.hpp"
#endif

namespace acl
{

json_node::json_node(ACL_JSON_NODE* node, json* json_ptr)
: node_me_(node)
, json_(json_ptr)
, dbuf_(json_ptr->get_dbuf())
, parent_(NULL)
, iter_(NULL)
, buf_(NULL)
, obj_(NULL)
{
	acl_assert(json_ptr);
	acl_assert(dbuf_);
}

json_node::~json_node(void)
{
	delete buf_;
}

const char* json_node::tag_name(void) const
{
	if (node_me_->ltag && ACL_VSTRING_LEN(node_me_->ltag) > 0) {
		return acl_vstring_str(node_me_->ltag);
	} else {
		return NULL;
	}
}

const char* json_node::get_text(void) const
{
	if (node_me_->text) {
		if (ACL_VSTRING_LEN(node_me_->text) > 0) {
			return acl_vstring_str(node_me_->text);
		} else {
			return "";
		}
	} else {
		return NULL;
	}
}

json_node* json_node::get_obj(void) const
{
	if (obj_ != NULL) {
		return obj_;
	}
	if (node_me_->tag_node == NULL) {
		return NULL;
	}

	const_cast<json_node*>(this)->obj_ =
		dbuf_->create<json_node>(node_me_->tag_node, json_);
	return obj_;
}

const char* json_node::get_string(void) const
{
	return get_text();
}

const acl_int64* json_node::get_int64(void) const
{
	if (!is_number()) {
		return NULL;
	}
	const char* txt = get_text();
	if (txt == NULL || *txt == 0) {
		return NULL;
	}
	const_cast<json_node*>(this)->node_val_.n = acl_atoi64(txt);
	return &node_val_.n;
}

const double* json_node::get_double(void) const
{
	if (!is_double()) {
		return NULL;
	}
	const char* txt = get_text();
	if (txt == NULL || *txt == 0) {
		return NULL;
	}
	const_cast<json_node*>(this)->node_val_.d = atof(txt);
	return &node_val_.d;
}

const bool* json_node::get_bool(void) const
{
	if (!is_bool()) {
		return NULL;
	}
	const char* txt = get_text();
	if (txt == NULL || *txt == 0) {
		return NULL;
	}
	const_cast<json_node*>(this)->node_val_.b =
		strcasecmp(txt, "true") == 0 ? true : false;
	return &node_val_.b;
}

bool json_node::is_string(void) const
{
	return (node_me_->type & ACL_JSON_T_A_STRING)
		|| (node_me_->type & ACL_JSON_T_STRING);
}

bool json_node::is_number(void) const
{
	return is_double();
}

bool json_node::is_double(void) const
{
	return  (node_me_->type & ACL_JSON_T_A_DOUBLE) ||
		(node_me_->type & ACL_JSON_T_DOUBLE) ||
		(node_me_->type & ACL_JSON_T_A_NUMBER) ||
		(node_me_->type & ACL_JSON_T_NUMBER);
}

bool json_node::is_bool(void) const
{
	return (node_me_->type & ACL_JSON_T_A_BOOL)
		|| (node_me_->type & ACL_JSON_T_BOOL);
}

bool json_node::is_null(void) const
{
	return (node_me_->type & ACL_JSON_T_A_NULL)
		|| (node_me_->type & ACL_JSON_T_NULL);
}

bool json_node::is_object(void) const
{
	if (node_me_->type & ACL_JSON_T_OBJ) {
		return true;
	} else {
		return false;
	}

	/*
	if (node_me_->tag_node == NULL)
		return false;
	if (node_me_->tag_node->type == ACL_JSON_T_OBJ)
		return true;
	else
		return false;
	*/
}

bool json_node::is_array(void) const
{
	if (node_me_->type & ACL_JSON_T_ARRAY) {
		return true;
	} else {
		return false;
	}
}

const char* json_node::get_type(void) const
{
	if (is_string()) {
		return "string";
	} else if (is_double()) {
		return "double";
	} else if (is_number()) {
		return "number";
	} else if (is_bool()) {
		return "bool";
	} else if (is_null()) {
		return "null";
	} else if (is_object()) {
		return "object";
	} else if (is_array()) {
		return "array";
	} else {
		return "unknown";
	}
}

bool json_node::set_tag(const char* name)
{
	if (name == NULL || *name == 0) {
		return false;
	}
	if (node_me_->ltag == NULL || ACL_VSTRING_LEN(node_me_->ltag) == 0) {
		return false;
	}
	acl_vstring_strcpy(node_me_->ltag, name);
	return true;
}

bool json_node::set_text(const char* text)
{
	if (text == NULL || *text == 0) {
		return false;
	}
	if (node_me_->text == NULL) {
		return false;
	}
	switch ((node_me_->type & ~ACL_JSON_T_LEAF)) {
	case ACL_JSON_T_NULL:
	case ACL_JSON_T_BOOL:
	case ACL_JSON_T_NUMBER:
	case ACL_JSON_T_DOUBLE:
	case ACL_JSON_T_STRING:
		break;
	default:
		return false;
	}
	acl_vstring_strcpy(node_me_->text, text);
	return true;
}

const string& json_node::to_string(string* out /* = NULL */) const
{
	if (out == NULL) {
		if (buf_ == NULL) {
			const_cast<json_node*>(this)->buf_ = NEW string(256);
		} else {
			const_cast<json_node*>(this)->buf_->clear();
		}
		out = const_cast<json_node*>(this)->buf_;
	}

	ACL_VSTRING* vbuf = out->vstring();
	(void) acl_json_node_build(node_me_, vbuf);
	return *out;
}

json_node& json_node::add_child(json_node* child, bool return_child /* = false */)
{
	ACL_JSON_NODE* node = child->get_json_node();
	// 先添加 child 至父节点中
	acl_json_node_add_child(node_me_, node);
	child->parent_ = this;
	if (return_child) {
		return *child;
	}
	return *this;
}

json_node& json_node::add_child(json_node& child, bool return_child /* = false */)
{
	return add_child(&child, return_child);
}

json_node& json_node::add_array(bool return_child /* = false */)
{
	return add_child(json_->create_array(), return_child);
}

json_node& json_node::add_child(bool as_array /* = false */,
	bool return_child /* = false */)
{
	return add_child(json_->create_node(as_array), return_child);
}

json_node& json_node::add_child(const char* tag, json_node* node,
	bool return_child /* = false */)
{
	return add_child(json_->create_node(tag, node), return_child);
}

json_node& json_node::add_child(const char* tag, json_node& node,
	bool return_child /* = false */)
{
	return add_child(tag, &node, return_child);
}

json_node& json_node::add_child(const char* tag, bool return_child /* = false */)
{
	json_node& node = json_->create_node();
	(void) add_child(tag, node);
	return return_child ? node : *this;
}

json_node& json_node::add_text(const char* tag, const char* value,
	bool return_child /* = false */)
{
	return add_child(json_->create_node(tag, value), return_child);
}

json_node& json_node::add_number(const char* tag, acl_int64 value,
	bool return_child /* = false */)
{
	return add_child(json_->create_node(tag, value), return_child);
}

json_node& json_node::add_double(const char* tag, double value,
	bool return_child /* = false */)
{
	return add_child(json_->create_double(tag, value), return_child);
}

json_node& json_node::add_double(const char* tag, double value, int precision,
	bool return_child /* = false */)
{
	return add_child(json_->create_double(tag, value, precision), return_child);
}

json_node& json_node::add_bool(const char* tag, bool value,
	bool return_child /* = false */)
{
	return add_child(json_->create_node(tag, value), return_child);
}

json_node& json_node::add_null(const char* tag, bool return_child /* = false */)
{
	return add_child(json_->create_null(tag), return_child);
}

json_node& json_node::add_array_text(const char* text,
	bool return_child /* = false */)
{
	return add_child(json_->create_array_text(text), return_child);
}

json_node& json_node::add_array_number(acl_int64 value,
	bool return_child /* = false */)
{
	return add_child(json_->create_array_number(value), return_child);
}

json_node& json_node::add_array_double(double value,
	bool return_child /* = false */)
{
	return add_child(json_->create_array_double(value), return_child);
}

json_node& json_node::add_array_bool(bool value,
	bool return_child /* = false */)
{
	return add_child(json_->create_array_bool(value), return_child);
}

json_node& json_node::add_array_null(bool return_child /* = false */)
{
	return add_child(json_->create_array_null(), return_child);
}

int json_node::detach(void)
{
	return acl_json_node_delete(node_me_);
}

json_node& json_node::get_parent(void) const
{
	if (parent_) {
		return *parent_;
	} else if (node_me_->parent == node_me_->json->root) {
		return json_->get_root();
	} else if (node_me_->parent == NULL) { // xxx: can this happen?
		return json_->get_root();
	}

	const_cast<json_node*>(this)->parent_ =
		dbuf_->create<json_node>(node_me_->parent, json_);
	return *parent_;
}

json_node* json_node::first_child(void)
{
	if (iter_ == NULL) {
		iter_ = (ACL_ITER*) dbuf_->dbuf_alloc(sizeof(ACL_ITER));
	}

	ACL_JSON_NODE* node = node_me_->iter_head(iter_, node_me_);
	if (node == NULL) {
		return NULL;
	}

	json_node* child = dbuf_->create<json_node>(node, json_);
	return child;
}

json_node* json_node::next_child(void)
{
	acl_assert(iter_);

	ACL_JSON_NODE* node = node_me_->iter_next(iter_, node_me_);
	if (node == NULL) {
		return NULL;
	}

	json_node* child = dbuf_->create<json_node>(node, json_);
	return child;
}

json_node* json_node::operator[](const char* tag)
{
	json_node* iter = first_child();
	while (iter) {
		const char* ptr = iter->tag_name();
		if (ptr != NULL && strcasecmp(ptr, tag) == 0) {
			return iter;
		}
		iter = next_child();
	}

	return NULL;
}

int json_node::depth(void) const
{
	return node_me_->depth;
}

int json_node::children_count(void) const
{
	return acl_ring_size(&node_me_->children);
}

void json_node::clear(void)
{
	if (buf_) {
		buf_->clear();
	}
}

json& json_node::get_json(void) const
{
	return *json_;
}

ACL_JSON_NODE* json_node::get_json_node() const
{
	return node_me_;
}

void json_node::set_json_node(ACL_JSON_NODE* node)
{
	node_me_ = node;
}

//////////////////////////////////////////////////////////////////////////

json::json(const char* data /* NULL */, dbuf_guard* dbuf /* NULL */)
{
	if (dbuf) {
		dbuf_ = dbuf;
	} else {
		dbuf_ = dbuf_internal_ = NEW dbuf_guard;
	}

	json_ = acl_json_dbuf_alloc(dbuf_->get_dbuf().get_dbuf());
	root_ = NULL;
	buf_  = NULL;
	iter_ = NULL;

	if (data && *data) {
		update(data);
	}
}

json::json(const json_node& node, dbuf_guard* dbuf /* NULL */)
{
	if (dbuf) {
		dbuf_ = dbuf;
	} else {
		dbuf_ = dbuf_internal_ = NEW dbuf_guard;
	}

	json_ = acl_json_dbuf_create(dbuf_->get_dbuf().get_dbuf(),
			node.get_json_node());
	root_ = dbuf_->create<json_node>(json_->root, this);
	buf_  = NULL;
	iter_ = NULL;
}

json::~json(void)
{
	delete dbuf_internal_;
	delete buf_;
}

json& json::part_word(bool on)
{
	if (on) {
		json_->flag |= ACL_JSON_FLAG_PART_WORD;
	} else {
		json_->flag &= ~ACL_JSON_FLAG_PART_WORD;
	}
	return *this;
}

const char* json::update(const char* data)
{
	return acl_json_update(json_, data);
}

bool json::finish(void)
{
	return acl_json_finish(json_) == 0 ? false : true;
}

json_node* json::getFirstElementByTagName(const char* tag) const
{
	ACL_JSON_NODE* n = acl_json_getFirstElementByTagName(json_, tag);
	if (n == NULL) {
		return NULL;
	}

	json_node* node = dbuf_->create<json_node>(n, const_cast<json*>(this));
	const_cast<json*>(this)->nodes_query_.push_back(node);
	return node;
}

json_node* json::operator[](const char* tag) const
{
	return getFirstElementByTagName(tag);
}

const std::vector<json_node*>& json::getElementsByTagName(const char* tag) const
{
	const_cast<json*>(this)->clear();

	ACL_ARRAY* a = acl_json_getElementsByTagName(json_, tag);
	if (a == NULL) {
		return nodes_query_;
	}

	ACL_ITER iter;
	acl_foreach(iter, a) {
		ACL_JSON_NODE *tmp = (ACL_JSON_NODE*) iter.data;
		json_node* node = dbuf_->create<json_node>
		        (tmp, const_cast<json*>(this));
		const_cast<json*>(this)->nodes_query_.push_back(node);
	}

	acl_json_free_array(a);
	return nodes_query_;
}

const std::vector<json_node*>& json::getElementsByTags(const char* tags) const
{
	const_cast<json*>(this)->clear();

	ACL_ARRAY* a = acl_json_getElementsByTags(json_, tags);
	if (a == NULL) {
		return nodes_query_;
	}

	ACL_ITER iter;
	acl_foreach(iter, a) {
		ACL_JSON_NODE *tmp = (ACL_JSON_NODE*) iter.data;
		json_node* node = dbuf_->create<json_node>
		        (tmp, const_cast<json*>(this));
		const_cast<json*>(this)->nodes_query_.push_back(node);
	}

	acl_json_free_array(a);
	return nodes_query_;
}

json_node* json::getFirstElementByTags(const char* tags) const
{
	const_cast<json*>(this)->clear();

	ACL_ARRAY* a = acl_json_getElementsByTags(json_, tags);
	if (a == NULL) {
		return NULL;
	}

	ACL_JSON_NODE* n = (ACL_JSON_NODE*) acl_array_index(a, 0);
	acl_assert(n);

	json_node* node = dbuf_->create<json_node>(n, const_cast<json*>(this));
	const_cast<json*>(this)->nodes_query_.push_back(node);

	acl_json_free_array(a);
	return node;
}

ACL_JSON* json::get_json(void) const
{
	return json_;
}

json_node& json::create_node(const char* tag, const char* value)
{
	ACL_JSON_NODE* node = acl_json_create_text(json_, tag, value);
	json_node* n = dbuf_->create<json_node>(node, this);
	return *n;
}

json_node& json::create_node(const char* tag, acl_int64 value)
{
	ACL_JSON_NODE* node = acl_json_create_int64(json_, tag, value);
	json_node* n = dbuf_->create<json_node>(node, this);
	return *n;
}

json_node& json::create_double(const char* tag, double value, int precision)
{
	ACL_JSON_NODE* node = acl_json_create_double2(json_, tag, value, precision);
	json_node* n = dbuf_->create<json_node>(node, this);
	return *n;
}

json_node& json::create_node(const char* tag, bool value)
{
	ACL_JSON_NODE* node = acl_json_create_bool(json_, tag, value ? 1 : 0);
	json_node* n = dbuf_->create<json_node>(node, this);
	return *n;
}

json_node& json::create_null(const char* tag)
{
	ACL_JSON_NODE* node = acl_json_create_null(json_, tag);
	json_node* n = dbuf_->create<json_node>(node, this);
	return *n;
}

json_node& json::create_array_text(const char* text)
{
	ACL_JSON_NODE* node = acl_json_create_array_text(json_, text);
	json_node* n = dbuf_->create<json_node>(node, this);
	return *n;
}

json_node& json::create_array_number(acl_int64 value)
{
	ACL_JSON_NODE* node = acl_json_create_array_int64(json_, value);
	json_node* n = dbuf_->create<json_node>(node, this);
	return *n;
}

json_node& json::create_array_double(double value)
{
	ACL_JSON_NODE* node = acl_json_create_array_double(json_, value);
	json_node* n = dbuf_->create<json_node>(node, this);
	return *n;
}

json_node& json::create_array_bool(bool value)
{
	ACL_JSON_NODE* node = acl_json_create_array_bool(json_, value);
	json_node* n = dbuf_->create<json_node>(node, this);
	return *n;
}

json_node& json::create_array_null(void)
{
	ACL_JSON_NODE* node = acl_json_create_array_null(json_);
	json_node* n = dbuf_->create<json_node>(node, this);
	return *n;
}

json_node& json::create_node(bool as_array /* = false */)
{
	if (as_array) {
		return create_array();
	}

	ACL_JSON_NODE* node = acl_json_create_obj(json_);
	json_node* n = dbuf_->create<json_node>(node, this);
	return *n;
}

json_node& json::create_array(void)
{
	ACL_JSON_NODE* node = acl_json_create_array(json_);
	json_node* n = dbuf_->create<json_node>(node, this);
	return *n;
}

json_node& json::create_node(const char* tag, json_node* node)
{
	ACL_JSON_NODE* tmp = acl_json_create_node(json_,
		tag, node->get_json_node());
	json_node* n = dbuf_->create<json_node>(tmp, this);
	return *n;
}

json_node& json::create_node(const char* tag, json_node& node)
{
	ACL_JSON_NODE* tmp = acl_json_create_node(json_,
		tag, node.get_json_node());
	json_node* n = dbuf_->create<json_node>(tmp, this);
	return *n;
}

json_node& json::duplicate_node(const json_node* node)
{
	ACL_JSON_NODE* tmp = acl_json_node_duplicate(json_,
		node->get_json_node());
	acl_assert(tmp);
	json_node* n = dbuf_->create<json_node>(tmp, this);
	return *n;
}

json_node& json::duplicate_node(const json_node& node)
{
	ACL_JSON_NODE* tmp = acl_json_node_duplicate(json_,
		node.get_json_node());
	acl_assert(tmp);
	json_node* n = dbuf_->create<json_node>(tmp, this);
	return *n;
}

json_node& json::get_root(void)
{
	if (root_) {
		root_->node_me_ = json_->root;
		return *root_;
	}
	root_ = dbuf_->create<json_node>(json_->root, this);
	return *root_;
}

json_node* json::first_node(void)
{
	if (iter_ == NULL) {
		iter_ = (ACL_ITER*) dbuf_->dbuf_alloc(sizeof(ACL_ITER));
	}
	ACL_JSON_NODE* node = json_->iter_head(iter_, json_);
	if (node == NULL) {
		return NULL;
	}
	json_node* n = dbuf_->create<json_node>(node, this);
	return n;
}

json_node* json::next_node(void)
{
	acl_assert(iter_);
	ACL_JSON_NODE* node = json_->iter_next(iter_, json_);
	if (node == NULL) {
		return NULL;
	}

	json_node* n = dbuf_->create<json_node>(node, this);
	return n;
}

const string& json::to_string(string* out /* = NULL */,
	bool add_space /* = false */) const
{
	if (out == NULL) {
		if (buf_ == NULL) {
			const_cast<json*>(this)->buf_ = NEW string(256);
		} else {
			const_cast<json*>(this)->buf_->clear();
		}
		out = const_cast<json*>(this)->buf_;
	}

	if (add_space) {
		const_cast<json*>(this)->json_->flag |= ACL_JSON_FLAG_ADD_SPACE;
	} else {
		const_cast<json*>(this)->json_->flag &= ~ACL_JSON_FLAG_ADD_SPACE;
	}

	ACL_VSTRING* vbuf = out->vstring();
	(void) acl_json_build(json_, vbuf);
	return *out;
}

void json::build_json(string& out, bool add_space /* = false */) const
{
	if (add_space) {
		const_cast<json*>(this)->json_->flag |= ACL_JSON_FLAG_ADD_SPACE;
	} else {
		const_cast<json*>(this)->json_->flag &= ~ACL_JSON_FLAG_ADD_SPACE;
	}

	ACL_VSTRING* buf = out.vstring();
	(void) acl_json_build(json_, buf);
}

void json::reset(void)
{
	clear();
	dbuf_->dbuf_reset();
	json_ = acl_json_dbuf_alloc(dbuf_->get_dbuf().get_dbuf());
	root_ = NULL;
	iter_ = NULL;
}

int json::push_pop(const char* in, size_t len acl_unused,
	string* out, size_t max /* = 0 */)
{
	if (in) {
		update(in);
	}
	if (out == NULL) {
		return 0;
	}
	if (max > 0 && len > max) {
		len = max;
	}
	out->append(in, len);
	return (int) len;
}

int json::pop_end(string* out acl_unused, size_t max /* = 0 */ acl_unused)
{
	return 0;
}

void json::clear(void)
{
	if (buf_) {
		buf_->clear();
	}

	nodes_query_.clear();
}

} // namespace acl
