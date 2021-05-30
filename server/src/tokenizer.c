//
// Created by danil on 28.05.2021.
//

#include "tokenizer.h"

vector* get_token_types() {

	vector* types = create_vector();
	types->push(types, "namespace");
	types->push(types, "type");
	types->push(types, "class");
	types->push(types, "enum");
	types->push(types, "interface");
	types->push(types, "struct");
	types->push(types, "typeParameter");
	types->push(types, "parameter");
	types->push(types, "variable");
	types->push(types, "property");
	types->push(types, "enumMember");
	types->push(types, "event");
	types->push(types, "function");
	types->push(types, "method");
	types->push(types, "macro");
	types->push(types, "keyword");
	types->push(types, "modifier");
	types->push(types, "comment");
	types->push(types, "string");
	types->push(types, "number");
	types->push(types, "regexp");
	types->push(types, "operator");
	return types;
}

vector* get_token_modifiers() {

	vector* types = create_vector();
	types->push(types, "declaration");
	types->push(types, "definition");
	types->push(types, "readonly");
	types->push(types, "static");
	types->push(types, "deprecated");
	types->push(types, "abstract");
	types->push(types, "async");
	types->push(types, "modification");
	types->push(types, "documentation");
	types->push(types, "defaultLibrary");

	return types;
}

int map_type_to_semantic_token(token_t type) {

	static token_t* mapper = NULL;
	if (mapper == NULL) {

		mapper = malloc(sizeof(token_t) * 64);

		/*
		 * Default types
		 * */
		mapper[Namespace] = Namespace;
		mapper[Type] = Type;
		mapper[Enum] = Enum;
		mapper[Interface] = Interface;
		mapper[Struct] = Struct;
		mapper[TypeParameter] = TypeParameter;
		mapper[Parameter] = Parameter;
		mapper[Variable] = Variable;
		mapper[Property] = Property;
		mapper[EnumMember] = EnumMember;
		mapper[Event] = Event;
		mapper[Function] = Function;
		mapper[Method] = Method;
		mapper[Macro] = Macro;
		mapper[Keyword] = Keyword;
		mapper[Modifier] = Modifier;
		mapper[Comment] = Comment;
		mapper[String] = String;
		mapper[Number] = Number;
		mapper[Regexp] = Regexp;
		mapper[Operator] = Operator;
		mapper[Unknown] = Unknown;

		/*
		 * Extra types
		 * */
		mapper[ArithmeticOperator] = Operator;
		mapper[LogicalOperator] = Operator;

		mapper[SpaceDelimiter] = Unknown;
		mapper[NewlineDelimiter] = Unknown;
		mapper[CommentDelimiter] = Modifier;
		mapper[StatementDelimiter] = Modifier;
		mapper[ArgumentDelimiter] = Modifier;
		mapper[ArgumentGroupDelimiter] = Unknown;
		mapper[EqualDelimiter] = Operator;
		mapper[IODelimiter] = Modifier;
		mapper[StringDelimiter] = String;

		mapper[IntegerVariable] = Variable;
		mapper[SinglePrecisionVariable] = Variable;
		mapper[DoublePrecisionVariable] = Variable;
		mapper[StringVariable] = Variable;
		mapper[ArrayIntegerVariable] = Variable;
		mapper[ArraySinglePrecisionVariable] = Variable;
		mapper[ArrayDoublePrecisionVariable] = Variable;
		mapper[ArrayStringVariable] = Variable;

		mapper[IntegerValue] = Number;
		mapper[SinglePrecisionValue] = Number;
		mapper[DoublePrecisionValue] = Number;

		mapper[LineNumber] = Number;
	}

	return mapper[type];
}

range* create_range_object(int line_l, int l, int line_r, int r) {

	range* rng = malloc(sizeof(range));
	rng->l = l;
	rng->r = r;
	rng->line_l = line_l;
	rng->line_r = line_r;
	return rng;
}

token* create_token(const char* str, int l, int r, int line_l, int line_r) {

	token* t = malloc(sizeof(token));
	t->str = copystr(str);
	t->l = l;
	t->r = r;
	t->line_l = line_l;
	t->line_r = line_r;
	t->kind = Unknown;
	return t;
}

//static token* get_token(tokenizer* self, const char* str, int l, int r, int line_l, int line_r) {
//	token* t = create_token(str, l, r, line_l, line_r);
//	set_token_type(self, t);
//	return t;
//}
//
//static void end_token(lvector* vect, tokenizer* self, char* token, int* sz, int l, int line) {
//	if (*sz) {
//		token[*sz] = 0;
//		vect->push(vect, get_token(self, copystr(token), l, l+(*sz)-1, line, line));
//		*sz = 0;
//	}
//}

static void concat_tokens(token* firstToken, token* secondToken) {

	char* str = concd(firstToken->str, secondToken->str, " ");
	free(firstToken->str), free(secondToken->str);
	firstToken->str = str;
	firstToken->r = secondToken->r;
	firstToken->line_r = secondToken->line_r;
	free_token_item(secondToken);
}

static token* split_token(token* self, int pos) {

	token* _token = create_token("", self->l + pos + 1, self->r, self->line_l, self->line_r);
	_token->str = substr(self->str, pos + 1, strlen(self->str) - 1);

	self->r = pos;
	char* temp = self->str;
	self->str = substr(temp, 0, pos);
	free(temp);

	return _token;
}

static int rem_grouping(tokenizer* self, lvector* nodes, lnode* node) {

	token* _token = node->val, * _prev_token = node->prev ? node->prev->val : NULL;
	if (!strcmp(_token->str, "rem")
		&& (!_prev_token || _prev_token->line_r < _token->line_l)) {
		token* next_token = node->next ? node->next->val : NULL;
		while (next_token && next_token->line_l == _token->line_r) {
			concat_tokens(_token, next_token);
			merge_nodes(nodes, node, node->next, _token);
			next_token = node->next ? node->next->val : NULL;
		}
		_token->kind = Comment;
		return 1;
	}

	return 0;
}

static int complex_keyword_grouping(tokenizer* self, lvector* nodes, lnode* node) {

	token* _token = node->val, * next_token = node->next ? node->next->val : NULL;
	if (!next_token)
		return 0;

	int vertex = self->keywords->vfind(0, self->keywords, _token->str);
	gwkeyword* walk_result = NULL;
	int space_edge = cast_char_to_wtree_edge(' ');
	if (self->keywords->tree[vertex][space_edge]
		&& (walk_result = self->keywords->vwalk(self->keywords->tree[vertex][space_edge], self->keywords,
			next_token->str))) {

		token* rtoken = split_token(next_token, strlen(walk_result->name) - (strlen(_token->str) + 1) - 1);
		node->next->val = rtoken;
		if (!strlen(rtoken->str)) extract_node(nodes, node->next);
		free_token_item(rtoken);

		concat_tokens(_token, next_token);
		_token->kind = walk_result->kind;

		return 1;
	}

	return 0;
}

static int semantic_grouping(tokenizer* self) {

	for (lnode* node = self->__tokens->first; node; node = node->next) {
		int result = rem_grouping(self, self->__tokens, node)
			|| complex_keyword_grouping(self, self->__tokens, node);
	}
}

static int is_keyword(tokenizer* self, lvector* nodes, lnode* node) {

	token* _token = node->val;
	gwkeyword* keyword_result = self->keywords->find(self->keywords, _token->str);
	if (keyword_result) {
		_token->kind = keyword_result->kind;
	}
	return _token->kind != Unknown;
}

static int is_line_number(tokenizer* self, lvector* nodes, lnode* node) {

	token* _token = node->val, * prev_token = node->prev ? node->prev->val : NULL;
	const char* str = _token->str;
	if (!prev_token || _token->line_l > prev_token->line_r) {
		if (matchb(str, "^[1-9][0-9]{0,16}$"))
			_token->kind = LineNumber;
	}
	return _token->kind != Unknown;
}

static int is_number(tokenizer* self, lvector* nodes, lnode* node) {

	token* _token = node->val;
	const char* str = _token->str;
	if (matchb(str, "^(+|-)?(0|([1-9][0-9]{0,4}))(!|#|%)?$")
		|| matchb(str, "^(+|-)?&H[0-8]{1,16}(!|#|%)?$")
		|| matchb(str, "^(+|-)?&O?[0-7]{1,16}(!|#|%)?$")) {
		_token->kind = IntegerValue;
	}
	else if (matchb(str, "^(+|-)?(0|([1-9][0-9]{0,6})).[0-9]{1,7}(!|#|%)?$")
		|| matchb(str, "^(+|-)?(0|([1-9][0-9]{0,6}))(.[0-9]{1,7})?e(+|-)?[0-9]{1,2}(!|#|%)?$")) {
		_token->kind = SinglePrecisionValue;
	}
	else if (matchb(str, "^(+|-)?(0|([1-9][0-9]{0,15}))(!|#|%)?$")
		|| matchb(str, "^(+|-)?(0|([1-9][0-9]{0,15})).[0-9]{1,16}(!|#|%)?$")
		|| matchb(str, "^(+|-)?(0|([1-9][0-9]{0,15}))(.[0-9]{1,16})?d(+|-)?[0-9]{1,2}(!|#|%)?$")) {
		_token->kind = DoublePrecisionValue;
	}

	/*
	 * В случае, если в конце был кастящий символ
	 * */
	if (_token->kind != Unknown) {
		char type = *(str + strlen(str) - 1);
		switch (type) {
		case '%':
			_token->kind = IntegerValue;
			break;
		case '!':
			_token->kind = SinglePrecisionValue;
			break;
		case '#':
			_token->kind = DoublePrecisionValue;
			break;
		default:
			break;
		}
	}

	return _token->kind != Unknown;
}

static void _set_variable_type_with_context(token* _token,
	token* prev_token, token* next_token,
	token_t deftype, token_t arrtype) {

	if ((prev_token && prev_token->kind == DimStatement)
		|| (next_token && next_token->kind == ArgumentGroupDelimiter && *next_token->str == '(')) {
		_token->kind = arrtype;
	}
	else {
		_token->kind = deftype;
	}
}

static gwkeyword* get_gwkeyword(char* name,
	char* type, char* purpose,char* syntax, token_t kind) {

	gwkeyword* keyword = malloc(sizeof(gwkeyword));
	keyword->name = copystr(name);
	keyword->type = copystr(type);
	keyword->purpose = copystr(purpose);
	keyword->syntax = copystr(syntax);
	keyword->kind = kind;

	return keyword;
}

static gwkeyword* create_new_variable(token* _token) {

	return get_gwkeyword(_token->str,
		"variable", "", _token->str, _token->kind);
}

static int is_new_variable(tokenizer* self, lvector* nodes, lnode* node) {

	token* _token = node->val,
		* prev_token = node->prev ? node->prev->val : NULL,
		* next_token = node->next ? node->next->val : NULL;
	const char* str = _token->str;
	/*
	 * Объвление переменной должно быть вначале строки или statement
	 * */
	if (prev_token && (prev_token->kind == LineNumber
		|| prev_token->kind == StatementDelimiter
		|| prev_token->kind == DimStatement)) {
		if (matchb(str, "^[a-z][a-z|0-9]{0,39}(\\$|%|!|#)?$")) {
			char type = *(str + strlen(str) - 1);
			switch (type) {
			case '$':
				_set_variable_type_with_context(_token, prev_token, next_token, StringVariable, ArrayStringVariable);
				break;
			case '%':
				_set_variable_type_with_context(_token,
					prev_token,
					next_token,
					IntegerVariable,
					ArrayIntegerVariable);
				break;
			case '!':
				_set_variable_type_with_context(_token, prev_token, next_token, SinglePrecisionVariable,
					ArraySinglePrecisionVariable);
				break;
			case '#':
				_set_variable_type_with_context(_token, prev_token, next_token, DoublePrecisionVariable,
					ArrayDoublePrecisionVariable);
				break;
			default:
				_set_variable_type_with_context(_token, prev_token, next_token, SinglePrecisionVariable,
					ArraySinglePrecisionVariable);
				break;
			}
			self->keywords->add(self->keywords, _token->str, create_new_variable(_token));
		}
	}
	return _token->kind != Unknown;
}

static int set_token_type(tokenizer* self, lvector* nodes, lnode* node) {

	token* _token = node->val;
	if (_token->kind != Unknown)
		return 1;

	const char* str = _token->str;
	if (!*str)
		return 0;

	int result = is_keyword(self, nodes, node)
		|| is_line_number(self, nodes, node)
		|| is_number(self, nodes, node)
		|| is_new_variable(self, nodes, node);

	return result;
}

static void set_token_types(tokenizer* self) {

	for (lnode* node = self->__tokens->first; node; node = node->next) {
		set_token_type(self, self->__tokens, node);
	}
}

static void finalize_token(tokenizer* self, token_t kind, int token_offset) {

	size_t len = self->__token_len;
	*(self->__token + len) = 0;
	token* _token = create_token(self->__token,
		self->__curr_char - len + 1 + token_offset, self->__curr_char+token_offset, self->__curr_line, self->__curr_line);
	_token->r = max(_token->r, _token->l);
	_token->kind = kind;
	self->__tokens->push(self->__tokens, _token);
	*(self->__token) = 0, self->__token_len = 0, self->state = Free;
}

static void finalize_delimiter(tokenizer* self, const int walk_result, token_t delimiter_kind) {

	char* name = walk_result ? self->delimiter_words->walk_path(self->delimiter_words, self->__data) :
				 wrapc(*(self->__data));
	for (int i = 0; i < strlen(name) - 1; ++i) self->__data++;
	if (!((delimiter_kind == SpaceDelimiter) || (delimiter_kind == NewlineDelimiter))) {
		token* _token = create_token(name, self->__curr_char, self->__curr_char + strlen(name) - 1,
			self->__curr_line, self->__curr_line);
		_token->kind = delimiter_kind;
		self->__tokens->push(self->__tokens, _token);
	}
	else if ((delimiter_kind == NewlineDelimiter) && self->state != StringReading) {
		self->__curr_line++, self->__curr_char = -1;
	}
	self->state = Free;
}

static lvector* tokenize(tokenizer* self, char* _str) {

	self->__tokens = create_lvector();
	self->state = Free;
	self->__data = _str, *(self->__token) = 0, self->__token_len = 0;
	self->__curr_line = 0, self->__curr_char = 0;

	while (self->state != EndOfData) {
		char character = *self->__data;
		int* walk_result = NULL;
		if (!character) {
			finalize_token(self, Unknown, -1);
			self->state = EndOfData;
		}
		else if (self->state == StringReading || self->state == CommentReading) {
			/*
			 * Во время прочтения строки - комментария ничего не надо парсить, кроме конца
			 * */
			if (self->state == CommentReading && (self->delimiters[character] == NewlineDelimiter
				|| *(walk_result = self->delimiter_words->walk(self->delimiter_words,
					self->__data)) == NewlineDelimiter)) {

				finalize_token(self, Comment, -1);
				finalize_delimiter(self, *walk_result, NewlineDelimiter);
			}
			else {
				*(self->__token + self->__token_len++) = character;
				if (self->state == StringReading && self->delimiters[character] == StringDelimiter)
					finalize_token(self, String, 0);
			}
		}
		else if (self->delimiters[character]
			|| (walk_result = self->delimiter_words->walk(self->delimiter_words, self->__data))) {
			finalize_token(self, Unknown, -1);
			ll delimiter_kind = self->delimiters[character] | (walk_result ? *walk_result : 0);
			if (!(delimiter_kind == StringDelimiter
				|| delimiter_kind == CommentDelimiter)) {

				finalize_delimiter(self, (walk_result ? *walk_result : 0), delimiter_kind);
			}
			else if (delimiter_kind == StringDelimiter) {
				*(self->__token + self->__token_len++) = character;
				self->state = StringReading;
			}
			else if (delimiter_kind == CommentDelimiter) {
				*(self->__token + self->__token_len++) = character;
				self->state = CommentReading;
			}
		}
		else {
			*(self->__token + self->__token_len++) = character;
		}
		self->__data++, self->__curr_char++;
	}

	semantic_grouping(self);
	set_token_types(self);

	return self->__tokens;
}

//static lvector* make_tokens(tokenizer* self, char* str) {
//
//	lvector* vect = create_lvector();
//	int line_index = 0, l = 0, sz = 0, line = 0;
//	char curr_token[1024];
//	while (*str) {
//		if (isspace(*str) || *str=='\"' || *str=='\'') {
//			end_token(vect, self, curr_token, &sz, l, line);
//			l = line_index;
//			if (*str=='\"') {
//				curr_token[sz++] = *(str++);
//				while (*str && *str!='\"')
//					curr_token[sz++] = *(str++);
//				end_token(vect, self, curr_token, &sz, l, line);
//			}
//			else if (*str=='\'' || *str=='\n' || (*(str+1) && *str=='\r' && *(str+1)=='\n')) {
//
//				while (*str && !(*str=='\n' || (*(str+1) && *str=='\r' && *(str+1)=='\n')))
//					curr_token[sz++] = *(str++);
//
//				end_token(vect, self, curr_token, &sz, l, line);
//				line++, line_index = -1;
//			}
//		}
//		else if (isprint(*str)) {
//			if (!sz) l = line_index;
//			curr_token[sz++] = *str;
//		}
//		str++, line_index++;
//	}
//	//last token
//	curr_token[sz] = 0;
//	vect->push(vect, get_token(self, copystr(curr_token), l, max(l+sz-1, 0), line, line));
//
//	return vect;
//}

static lvector* make_tokens_with_range(tokenizer* self, char* text, range* rng) {

	lvector* tokens = self->make_tokens(self, text);
	lvector* result = create_lvector();
	iterator* it = tokens->iterator(tokens);
	while (it->has_next(it)) {
		token* tkn = it->get_next(it);
		if ((tkn->line_l > rng->line_l || (tkn->line_l == rng->line_l && tkn->l >= rng->l))
		&& (tkn->line_r < rng->line_r || (tkn->line_r == rng->line_r && tkn->r <= rng->r)))
			result->push(result, tkn);
	}
	free_lvector_no_values(tokens);

	return result;
}

static vector* make_tokens_with_lines(tokenizer* self, char* text) {

	vector* lines = create_vector();
	int curr_line = -1;
	lvector* tokens = self->make_tokens(self, text);
	iterator* it = tokens->iterator(tokens);

	while (it->has_next(it)) {
		token* tkn = it->get_next(it);
		while (curr_line < tkn->line_l)
			lines->push(lines, create_vector()), curr_line++;
		vector* last_line = lines->last(lines);
		last_line->push(last_line, tkn);
	}
	free_lvector_no_values(tokens);

	return lines;
}

token* find_token(vector* tokens_line, int pos) {

	iterator* it = tokens_line->iterator(tokens_line);
	token* result_tkn = NULL;
	while (it->has_next(it)) {
		token* tkn = it->get_next(it);
		if (tkn->l <= pos)
			result_tkn = tkn;
		else
			break;
	}
	return result_tkn;
}

static wtree* load_keywords(json_value* config) {

	wtree* words = create_wtree();
	json_value* keywords = get_by_name(config, "keywords");
	for (int i = 0; i < keywords->u.array.length; ++i) {
		json_value* keyword = keywords->u.array.values[i];
		gwkeyword *payload = get_gwkeyword(
			get_by_name(keyword, "name")->u.string.ptr,
			get_by_name(keyword, "type")->u.string.ptr,
			get_by_name(keyword, "purpose")->u.string.ptr,
			get_by_name(keyword, "syntax")->u.string.ptr,
			get_by_name(keyword, "kind")->u.integer);
		words->add(words, get_by_name(keyword, "name")->u.string.ptr, payload);
	}

	return words;
}

void fill_delimiters(tokenizer* t) {

	t->delimiters = malloc(sizeof(token_t) * MAX_SYMBOLS);
	memset(t->delimiters, 0, sizeof(token_t) * MAX_SYMBOLS);

	t->delimiter_words = create_wtree_sized(64);

	/*
	 * Arithmetic operators
	 * */
	t->delimiters['+'] = ArithmeticOperator;
	t->delimiters['-'] = ArithmeticOperator;
	t->delimiters['*'] = ArithmeticOperator;
	t->delimiters['\\'] = ArithmeticOperator;
	t->delimiters['/'] = ArithmeticOperator;
	t->delimiters['^'] = ArithmeticOperator;

	/*
	 * New line
	 * */
	t->delimiters['\n'] = NewlineDelimiter;
	t->delimiters['\r'] = NewlineDelimiter;
	t->delimiter_words->add(t->delimiter_words, "\r\n", wrapi(NewlineDelimiter));

	/*
	 * Spaces
	 * */
	t->delimiters[' '] = SpaceDelimiter;
	t->delimiters['\t'] = SpaceDelimiter;

	/*
	 * Semantic delimiters
	 * */
	t->delimiters[':'] = StatementDelimiter;
	t->delimiters[','] = ArgumentDelimiter;
	t->delimiters['('] = ArgumentGroupDelimiter;
	t->delimiters[')'] = ArgumentGroupDelimiter;
	t->delimiters[';'] = IODelimiter;
	t->delimiters['\''] = CommentDelimiter;
	t->delimiters['\"'] = StringDelimiter;

	/*
	 * Logical delimiters
	 * */
	t->delimiters['='] = EqualDelimiter;
	t->delimiters['>'] = LogicalOperator;
	t->delimiters['<'] = LogicalOperator;
	t->delimiter_words->add(t->delimiter_words, "<>", wrapi(LogicalOperator));
	t->delimiter_words->add(t->delimiter_words, "<=", wrapi(LogicalOperator));
	t->delimiter_words->add(t->delimiter_words, ">=", wrapi(LogicalOperator));
}

tokenizer* create_tokenizer(json_value* config) {

	tokenizer* t = malloc(sizeof(tokenizer));
	t->keywords = load_keywords(config);
	fill_delimiters(t);
	t->__token = malloc(sizeof(char) * 1024);
	t->make_tokens = tokenize;
	t->make_tokens_with_lines = make_tokens_with_lines;
	t->make_tokens_with_range = make_tokens_with_range;
	return t;
}

void free_token_item(token* t) {

	free(t->str);
	free(t);
}

void free_token_items(lvector* items) {

	iterator *it = items->iterator(items);
	while(it->has_next(it)) {
		free_token_item(it->get_next(it));
	}
	free_lvector_no_values(items);
}

void free_tokens(vector* tokens) {

	iterator* it = tokens->iterator(tokens);
	while (it->has_next(it))
		free_vector(it->get_next(it));
	free_vector_no_values(tokens);
}

void free_tokenizer(tokenizer* t) {

	free_wtree(t->keywords);
	free_wtree(t->delimiter_words);
	free(t->delimiters);
	free(t->__token);
	free(t);
}
