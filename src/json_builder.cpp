#include "json_builder.h"

namespace json {

	using namespace std::string_literals;

	namespace detail {
		Node ValueToNode(const Node::Value& value) {
			return std::visit(
				[](const auto& v) -> Node {
					return Node(v);
				},
				value
			);
		}
	} // namespace detail

	// === class Builder ===
	Node Builder::Build() {
		if (!nodes_stack_.empty()) {
			throw std::logic_error("JSON object is incomplete"s);
		}
		if (root_.IsNull()) {
			throw std::logic_error("JSON object is not set"s);
		}
		return std::move(root_);
	}

	Builder::DictValueContext Builder::Key(std::string key) {
		Node::Value& current_value = GetCurrentValue();
		if (!std::holds_alternative<Dict>(current_value)) {
			throw std::logic_error("Key called outside of the Dict"s);
		}
		Dict& dict = std::get<Dict>(current_value);
		Node& node = dict[std::move(key)];
		nodes_stack_.push_back(&node);
		return DictValueContext(*this);
	}

	Builder::BaseContext Builder::Value(Node::Value value) {
		AddNode(std::move(value), /* is_container = */ false);
		return BaseContext(*this);
	}

	Builder::DictItemContext Builder::StartDict() {
		AddNode(Dict{}, /* is_container = */ true);
		return DictItemContext{ *this };
	}

	Builder::ArrayItemContext Builder::StartArray() {
		AddNode(Array{}, /* is_container = */ true);
		return ArrayItemContext{ *this };
	}

	Builder::BaseContext Builder::EndDict() {
		if (nodes_stack_.empty() || !nodes_stack_.back()->IsMap()) {
			throw std::logic_error("EndDict called outside of the Dict");
		}
		nodes_stack_.pop_back();
		return BaseContext(*this);
	}

	Builder::BaseContext Builder::EndArray() {
		if (nodes_stack_.empty() || !nodes_stack_.back()->IsArray()) {
			throw std::logic_error("EndArray called outside of the Array");
		}
		nodes_stack_.pop_back();
		return BaseContext(*this);
	}

	Node::Value& Builder::GetCurrentValue() {
		if (nodes_stack_.empty()) {
			throw std::logic_error("Node stack is empty, all containers are closed"s);
		}
		return const_cast<Node::Value&>(nodes_stack_.back()->GetValue());
	}

	void Builder::AddNode(Node::Value value, bool is_container) {
		if (root_.IsNull() && nodes_stack_.empty()) {
			root_ = std::move(detail::ValueToNode(value));
			if (is_container) {
				nodes_stack_.push_back(&root_);
			}
			return;
		}

		Node::Value& current_value = GetCurrentValue();
		if (std::holds_alternative<Array>(current_value)) {
			Array& arr = std::get<Array>(current_value);
			arr.emplace_back(detail::ValueToNode(std::move(value)));
			if (is_container) {
				nodes_stack_.push_back(&arr.back());
			}
		}
		else if (std::holds_alternative<std::nullptr_t>(current_value)) {
			current_value = std::move(value);
			if (!is_container) {
				nodes_stack_.pop_back();
			}
		}
		else {
			throw std::logic_error("AddNode called in invalid context");
		}
	}

	// === class BaseContext ===
	Builder::BaseContext::BaseContext(Builder& builder) :
		builder_(builder) {
	}
	Node Builder::BaseContext::Build() {
		return builder_.Build();
	}
	Builder::DictValueContext Builder::BaseContext::Key(std::string key) {
		return builder_.Key(std::move(key));
	}
	Builder::BaseContext Builder::BaseContext::Value(Node::Value value) {
		return builder_.Value(std::move(value));
	}
	Builder::DictItemContext Builder::BaseContext::StartDict() {
		return builder_.StartDict();
	}
	Builder::ArrayItemContext Builder::BaseContext::StartArray() {
		return builder_.StartArray();
	}
	Builder::BaseContext Builder::BaseContext::EndDict() {
		return builder_.EndDict();
	}
	Builder::BaseContext Builder::BaseContext::EndArray() {
		return builder_.EndArray();
	}

	// === class DictValueContext ===
	Builder::DictValueContext::DictValueContext(Builder& builder)
		: BaseContext(builder) {
	}
	Builder::DictItemContext Builder::DictValueContext::Value(Node::Value value) {
		return BaseContext::Value(std::move(value));
	}

	// === class DictItemContext ===
	Builder::DictItemContext::DictItemContext(BaseContext base)
		: BaseContext(base) {
	}

	// === class ArrayItemContext ===
	Builder::ArrayItemContext::ArrayItemContext(BaseContext base)
		: BaseContext(base) {
	}
	Builder::ArrayItemContext Builder::ArrayItemContext::Value(Node::Value value) {
		return BaseContext::Value(std::move(value));
	}

} // namespace json