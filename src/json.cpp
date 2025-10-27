#include "json.h"

namespace json {
	using namespace std::literals;

	// ===== class Node =====

	Node::Node(Value value) :
		value_(std::move(value)) {
	}
	const Node::Value& Node::GetValue() const {
		return value_;
	}
	bool Node::IsInt() const {
		return std::holds_alternative<int>(value_);
	}
	bool Node::IsDouble() const {
		return std::holds_alternative<double>(value_)
			|| std::holds_alternative<int>(value_);
	}
	bool Node::IsPureDouble() const {
		return std::holds_alternative<double>(value_);
	}
	bool Node::IsBool() const {
		return std::holds_alternative<bool>(value_);
	}
	bool Node::IsString() const {
		return std::holds_alternative<std::string>(value_);
	}
	bool Node::IsNull() const {
		return std::holds_alternative<std::nullptr_t>(value_);
	}
	bool Node::IsArray() const {
		return std::holds_alternative<Array>(value_);
	}
	bool Node::IsMap() const {
		return std::holds_alternative<Dict>(value_);
	}

	int Node::AsInt() const {
		if (IsInt()) {
			return std::get<int>(value_);
		}
		throw std::logic_error("Not an int"s);
	}
	bool Node::AsBool() const {
		if (IsBool()) {
			return std::get<bool>(value_);
		}
		throw std::logic_error("Not a bool"s);
	}
	double Node::AsDouble() const {
		if (IsDouble()) {
			if (IsPureDouble()) {
				return std::get<double>(value_);
			}
			return static_cast<double>(std::get<int>(value_));
		}
		throw std::logic_error("Not a double"s);
	}
	const std::string& Node::AsString() const {
		if (IsString()) {
			return std::get<std::string>(value_);
		}
		throw std::logic_error("Not a string"s);
	}
	const Array& Node::AsArray() const {
		if (IsArray()) {
			return std::get<Array>(value_);
		}
		throw std::logic_error("Not an array"s);
	}
	const Dict& Node::AsMap() const {
		if (IsMap()) {
			return std::get<Dict>(value_);
		}
		throw std::logic_error("Not a map"s);
	}

	bool Node::operator==(const Node& other) const {
		return value_ == other.value_;
	}
	bool Node::operator!=(const Node& other) const {
		return !(*this == other);
	}

	// ===== class Document =====

	Document::Document(Node root)
		: root_(std::move(root)) {
	}
	const Node& Document::GetRoot() const {
		return root_;
	}
	bool Document::operator==(const Document& other) {
		return root_ == other.GetRoot();
	}
	bool Document::operator!=(const Document& other) {
		return !(*this == other);
	}

	// ===== LOAD and PRINT =====

	namespace {

		// ----- Load -----

		Node LoadNode(std::istream& input);

		Node LoadString(std::istream& input) {
			std::string str;
			char c;
			while (input.get(c)) {
				if (c == '"') {
					break;
				}
				if (c == '\\') {
					input.get(c);
					switch (c) {
					case 'n': str += '\n'; break;
					case 'r': str += '\r'; break;
					case 't': str += '\t'; break;
					case '"': str += '"'; break;
					case '\\': str += '\\'; break;
					default: throw ParsingError("Invalid escape character");
					}
				}
				else {
					str += c;
				}
			}

			if (!input) {
				throw ParsingError("Unexpected end (string)");
			}

			return Node(std::move(str));
		}

		std::string LoadLiteral(std::istream& input) {
			std::string s;
			while (std::isalpha(input.peek())) {
				s.push_back(static_cast<char>(input.get()));
			}
			return s;
		}

		Node LoadBool(std::istream& input) {
			const auto s = LoadLiteral(input);
			if (s == "true"sv) {
				return Node{ true };
			}
			else if (s == "false"sv) {
				return Node{ false };
			}
			else {
				throw ParsingError("Failed to parse '"s + s + "' as bool"s);
			}
		}

		Node LoadNull(std::istream& input) {
			if (auto literal = LoadLiteral(input); literal == "null"sv) {
				return Node{ nullptr };
			}
			else {
				throw ParsingError("Failed to parse '"s + literal + "' as null"s);
			}
		}

		Node LoadNumber(std::istream& input) {
			std::string parsed_num;

			// Считывает в parsed_num очередной символ из input
			auto read_char = [&parsed_num, &input] {
				parsed_num += static_cast<char>(input.get());
				if (!input) {
					throw ParsingError("Failed to read number from stream"s);
				}
				};

			// Считывает одну или более цифр в parsed_num из input
			auto read_digits = [&input, read_char] {
				if (!std::isdigit(input.peek())) {
					throw ParsingError("A digit is expected"s);
				}
				while (std::isdigit(input.peek())) {
					read_char();
				}
				};

			if (input.peek() == '-') {
				read_char();
			}
			// Парсим целую часть числа
			if (input.peek() == '0') {
				read_char();
				// После 0 в JSON не могут идти другие цифры
			}
			else {
				read_digits();
			}

			bool is_int = true;
			// Парсим дробную часть числа
			if (input.peek() == '.') {
				read_char();
				read_digits();
				is_int = false;
			}

			// Парсим экспоненциальную часть числа
			if (int ch = input.peek(); ch == 'e' || ch == 'E') {
				read_char();
				if (ch = input.peek(); ch == '+' || ch == '-') {
					read_char();
				}
				read_digits();
				is_int = false;
			}

			try {
				if (is_int) {
					// Сначала пробуем преобразовать строку в int
					try {
						return std::stoi(parsed_num);
					}
					catch (...) {
						// В случае неудачи, например, при переполнении
						// код ниже попробует преобразовать строку в double
					}
				}
				return std::stod(parsed_num);
			}
			catch (...) {
				throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
			}
		}

		Node LoadArray(std::istream& input) {
			Array arr;

			for (char c; input >> c && c != ']';) {
				if (c != ',') {
					input.putback(c);
				}
				arr.push_back(LoadNode(input));
			}
			if (!input) {
				throw ParsingError("Array parsing error"s);
			}
			return Node(std::move(arr));
		}

		Node LoadDict(std::istream& input) {
			Dict dict;

			for (char c; input >> c && c != '}';) {
				if (c == '"') {
					std::string key = LoadString(input).AsString();
					if (input >> c && c == ':') {
						if (dict.find(key) != dict.end()) {
							throw ParsingError("Duplicate key '"s + key + "' have been found");
						}
						dict.emplace(std::move(key), LoadNode(input));
					}
					else {
						throw ParsingError(": is expected but '"s + c + "' has been found"s);
					}
				}
				else if (c != ',') {
					throw ParsingError(R"(',' is expected but ')"s + c + "' has been found"s);
				}
			}
			if (!input) {
				throw ParsingError("Dictionary parsing error"s);
			}
			return Node(std::move(dict));
		}

		Node LoadNode(std::istream& input) {
			char c;
			if (!(input >> c)) {
				throw ParsingError("Unexpected EOF"s);
			}

			switch (c) {
			case '[': return LoadArray(input);
			case '{': return LoadDict(input);
			case '"': return LoadString(input);
			case 't': [[fallthrough]];
			case 'f': input.putback(c); return LoadBool(input);
			case 'n': input.putback(c); return LoadNull(input);
			default:
				if (std::isdigit(c) || c == '-') {
					input.putback(c);
					return LoadNumber(input);
				}
				throw ParsingError("Unexpected character: "s + c);
			}
		}

		// ----- Print -----

		struct PrintContext {
			std::ostream& out;
			int indent_step = 4;
			int indent = 0;

			void PrintIndent() const {
				for (int i = 0; i < indent; ++i) {
					out.put(' ');
				}
			}

			PrintContext Indented() const {
				return { out, indent_step, indent_step + indent };
			}
		};

		void PrintString(const std::string& value, std::ostream& out) {
			out.put('"');
			for (char c : value) {
				switch (c) {
				case '\\': out << "\\\\"sv; break;
				case '\n': out << "\\n"sv; break;
				case '\r': out << "\\r"sv; break;
				case '\t': out << "\\t"sv; break;
				case '"': out << "\\\""sv; break;
				default: out.put(c);; break;
				}
			}
			out.put('"');
		}

		template <typename Value>
		void PrintValue(const Value& value, const PrintContext& ctx) {
			ctx.out << value;
		}

		template <>
		void PrintValue<std::nullptr_t>(const std::nullptr_t&, const PrintContext& ctx) {
			ctx.out << "null"sv;
		}

		template <>
		void PrintValue<bool>(const bool& value, const PrintContext& ctx) {
			ctx.out << (value ? "true"sv : "false"sv);
		}

		template <>
		void PrintValue<std::string>(const std::string& value, const PrintContext& ctx) {
			PrintString(value, ctx.out);
		}

		void PrintNode(const Node& node, const PrintContext& ctx);

		template <>
		void PrintValue<Array>(const Array& nodes, const PrintContext& ctx) {
			std::ostream& out = ctx.out;
			out << "[\n"sv;
			bool is_first = true;
			auto inner_ctx = ctx.Indented();
			for (const Node& node : nodes) {
				if (!is_first) {
					out << ",\n"sv;
				}
				is_first = false;
				inner_ctx.PrintIndent();
				PrintNode(node, inner_ctx);
			}
			out.put('\n');
			ctx.PrintIndent();
			out.put(']');
		}

		template <>
		void PrintValue<Dict>(const Dict& nodes, const PrintContext& ctx) {
			std::ostream& out = ctx.out;
			out << "{\n"sv;
			bool is_first = true;
			auto inner_ctx = ctx.Indented();
			for (const auto& [key, node] : nodes) {
				if (!is_first) {
					ctx.out << ",\n"sv;
				}
				is_first = false;
				inner_ctx.PrintIndent();
				PrintString(key, out);
				out << ": "sv;
				PrintNode(node, inner_ctx);
			}
			out.put('\n');
			ctx.PrintIndent();
			out.put('}');
		}

		void PrintNode(const Node& node, const PrintContext& ctx) {
			std::visit(
				[&ctx](const auto& value) {
					PrintValue(value, ctx);
				},
				node.GetValue());
		}

	} // namespace

	Document Load(std::istream& input) {
		return Document{ LoadNode(input) };
	}

	void Print(const Document& doc, std::ostream& output) {
		PrintNode(doc.GetRoot(), PrintContext{ output });
	}

}  // namespace json