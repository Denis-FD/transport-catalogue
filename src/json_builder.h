#pragma once

#include "json.h"

namespace json {

	class Builder {
	private:
		class BaseContext;
		class DictValueContext;
		class DictItemContext;
		class ArrayItemContext;

	public:
		Builder() = default;
		Node Build();
		DictValueContext Key(std::string key);
		BaseContext Value(Node::Value value);
		DictItemContext StartDict();
		ArrayItemContext StartArray();
		BaseContext EndDict();
		BaseContext EndArray();

	private:
		Node root_;
		std::vector<Node*> nodes_stack_;

		Node::Value& GetCurrentValue();
		void AddNode(Node::Value value, bool is_container);

		class BaseContext {
		public:
			BaseContext(Builder& builder);
			Node Build();
			DictValueContext Key(std::string key);
			BaseContext Value(Node::Value value);
			DictItemContext StartDict();
			ArrayItemContext StartArray();
			BaseContext EndDict();
			BaseContext EndArray();
		private:
			Builder& builder_;
		};

		class DictValueContext : public BaseContext {
		public:
			DictValueContext(Builder& builder);
			DictItemContext Value(Node::Value value);
			Node Build() = delete;
			DictValueContext Key(std::string key) = delete;
			BaseContext EndDict() = delete;
			BaseContext EndArray() = delete;
		};

		class DictItemContext : public BaseContext {
		public:
			DictItemContext(BaseContext base);
			Node Build() = delete;
			BaseContext Value(Node::Value value) = delete;
			BaseContext EndArray() = delete;
			DictItemContext StartDict() = delete;
			ArrayItemContext StartArray() = delete;
		};

		class ArrayItemContext : public BaseContext {
		public:
			ArrayItemContext(BaseContext base);
			ArrayItemContext Value(Node::Value value);
			Node Build() = delete;
			DictValueContext Key(std::string key) = delete;
			BaseContext EndDict() = delete;
		};
	};

}  // namespace json