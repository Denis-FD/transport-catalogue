#pragma once

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace json {

	class Node;
	using Dict = std::map<std::string, Node>;
	using Array = std::vector<Node>;

	class ParsingError : public std::runtime_error {
	public:
		using runtime_error::runtime_error;
	};

	class Node final {
	public:
		using Value = std::variant<std::nullptr_t, Array, Dict,
			bool, int, double, std::string>;

		Node() = default;

		template <typename T>
		Node(T value);

		explicit Node(Value value);

		const Value& GetValue() const;

		bool IsInt() const;
		bool IsDouble() const;
		bool IsPureDouble() const;
		bool IsBool() const;
		bool IsString() const;
		bool IsNull() const;
		bool IsArray() const;
		bool IsMap() const;

		int AsInt() const;
		bool AsBool() const;
		double AsDouble() const;
		const std::string& AsString() const;
		const Array& AsArray() const;
		const Dict& AsMap() const;

		bool operator==(const Node& other) const;
		bool operator!=(const Node& other) const;

	private:
		Value value_;
	};

	class Document {
	public:
		explicit Document(Node root);
		const Node& GetRoot() const;
		bool operator==(const Document& other);
		bool operator!=(const Document& other);

	private:
		Node root_;
	};

	Document Load(std::istream& input);

	void Print(const Document& doc, std::ostream& output);

	template <typename T>
	Node::Node(T value) : value_(std::move(value)) {
	}

}  // namespace json