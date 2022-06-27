#include "Syntatic.h"
#include <memory>

void SyntaticAnalyzer::print_error() {
    std::cout << "Error: Unexpected token at position " << lexic.get_file_position() << std::endl;
    has_error = true;
}

Token* SyntaticAnalyzer::get_next_token() {
    if (returned_token) {
        returned_token = false;
        return token_read;
    }

    token_read = lexic.get_next_token();

    while (token_read->token_type == Token::INVALID_TOKEN) {
        std::cout << "Error: Invalid token at position " << lexic.get_file_position() << std::endl;
        has_error = true;
        token_read = lexic.get_next_token();
    }

    return token_read;
}

void SyntaticAnalyzer::return_token() {
    returned_token = true; 
}

void SyntaticAnalyzer::eat_until(std::list<Token::TokenType> tokens) {
    return_token(); 
    Token* token = get_next_token();
    while ( std::find(tokens.begin(), tokens.end(), token->token_type) == tokens.end() &&
            token->token_type != Token::END_OF_FILE) {
        token = get_next_token();
    }

    return_token();   
}

std::unique_ptr<Constant> SyntaticAnalyzer::parseConstant() {
    Token* token = get_next_token();

    if (token->token_type == Token::CONST_INT ||
        token->token_type == Token::CONST_FLOAT ||
        token->token_type == Token::CONST_CHAR) {
            return std::make_unique<Constant>((ValueType*) token);
        }

    print_error();
    eat_until(Constant::follow);

    return nullptr;
}

std::unique_ptr<MulOp> SyntaticAnalyzer::parseMulOp() {
    Token* token = get_next_token();

    switch (token->token_type) {
        case Token::MUL:
            return std::make_unique<MulOp>(MulOp::MUL);
        case Token::DIV:
            return std::make_unique<MulOp>(MulOp::DIV);
        case Token::AND:
            return std::make_unique<MulOp>(MulOp::AND);
        default:
            print_error();
            eat_until(MulOp::follow);
            return nullptr;
    }
}

std::unique_ptr<AddOp> SyntaticAnalyzer::parseAddOp() {
    Token* token = get_next_token();

    switch (token->token_type) {
        case Token::ADD:
            return std::make_unique<AddOp>(AddOp::ADD);
        case Token::SUB:
            return std::make_unique<AddOp>(AddOp::SUB);
        case Token::OR:
            return std::make_unique<AddOp>(AddOp::OR);
        default:
            print_error();
            eat_until(AddOp::follow);
            return nullptr;
    }
}

std::unique_ptr<RelOp> SyntaticAnalyzer::parseRelOp() {
    Token* token = get_next_token();

    switch (token->token_type) {
        case Token::COMP_EQ:
            return std::make_unique<RelOp>(RelOp::EQUALS);
        case Token::COMP_NE:
            return std::make_unique<RelOp>(RelOp::NOT_EQUALS);
        case Token::COMP_GT:
            return std::make_unique<RelOp>(RelOp::GREATER_THAN);
        case Token::COMP_LT:
            return std::make_unique<RelOp>(RelOp::LESS_THAN);
        case Token::COMP_GE:
            return std::make_unique<RelOp>(RelOp::GREATER_THAN_OR_EQUAL);
        case Token::COMP_LE:
            return std::make_unique<RelOp>(RelOp::LESS_THAN_OR_EQUAL);
        default:
            print_error();
            eat_until(RelOp::follow);
            return nullptr;
    }
}

std::unique_ptr<Factor> SyntaticAnalyzer::parseFactor() {
    Token* token = get_next_token();

    std::unique_ptr<Expression> expr;

    switch (token->token_type) {
        case Token::ID:
            return std::make_unique<Factor>((TokenId*) token);
        case Token::CONST_INT:
        case Token::CONST_FLOAT:
        case Token::CONST_CHAR:
            return_token();
            return std::make_unique<Factor>(parseConstant());
        case Token::OPEN_BRACES:
            expr = parseExpression();

            token = get_next_token();
            if (token->token_type != Token::CLOSE_BRACES) {
                print_error();
                eat_until(Factor::follow);
            }

            return std::make_unique<Factor>(std::move(expr));
        default:
            print_error();
            eat_until(Factor::follow);
            return nullptr;
    }
}

std::unique_ptr<FactorA> SyntaticAnalyzer::parseFactorA() {
    Token* token = get_next_token();

    switch (token->token_type) {
        case Token::NOT:
            return std::make_unique<FactorA>(parseFactor(), FactorA::Not);
        case Token::SUB:
            return std::make_unique<FactorA>(parseFactor(), FactorA::Sub);
        default:
            return_token();
            return std::make_unique<FactorA>(parseFactor(), FactorA::Base);
    }
}

std::unique_ptr<Term> SyntaticAnalyzer::parseTerm() {
    Token* token = get_next_token();

    std::unique_ptr<MulOp> mulOp;
    std::unique_ptr<FactorA> factorA;
    std::unique_ptr<Term> term;

    switch (token->token_type) {
        case Token::MUL:
        case Token::DIV:
        case Token::AND:
            return_token();
            mulOp = parseMulOp();
            factorA = parseFactorA();
            term = parseTerm();
            return std::make_unique<Term>(std::move(mulOp), std::move(factorA), std::move(term));
        default:
            return_token();
            return std::make_unique<Term>();
    }
}

std::unique_ptr<TermA> SyntaticAnalyzer::parseTermA() {
    std::unique_ptr<FactorA> factorA = parseFactorA();
    std::unique_ptr<Term> term = parseTerm();
    return std::make_unique<TermA>(std::move(factorA), std::move(term));
}

std::unique_ptr<SimpleExpr> SyntaticAnalyzer::parseSimpleExpr() {
    Token* token = get_next_token();

    std::unique_ptr<AddOp> addOp;
    std::unique_ptr<TermA> termA;
    std::unique_ptr<SimpleExpr> simpleExpr;

    switch (token->token_type) {
        case Token::ADD:
        case Token::SUB:
        case Token::OR:
            return_token();
            addOp = parseAddOp();
            termA = parseTermA();
            simpleExpr = parseSimpleExpr();
            return std::make_unique<SimpleExpr>(std::move(addOp), std::move(termA), std::move(simpleExpr));
        default:
            return_token();
            return std::make_unique<SimpleExpr>();
    }
}

std::unique_ptr<SimpleExprA> SyntaticAnalyzer::parseSimpleExprA() {
    std::unique_ptr<TermA> termA = parseTermA();
    std::unique_ptr<SimpleExpr> simpleExpr = parseSimpleExpr();
    return std::make_unique<SimpleExprA>(std::move(termA), std::move(simpleExpr));
}

std::unique_ptr<Expression> SyntaticAnalyzer::parseExpression() {
    std::unique_ptr<SimpleExprA> simpleExprA = parseSimpleExprA();
    std::unique_ptr<RelOp> relOp;
    std::unique_ptr<SimpleExprA> simpleExprB;

    Token* token = get_next_token();

    switch (token->token_type) {
        case Token::COMP_EQ:
        case Token::COMP_NE:
        case Token::COMP_GT:
        case Token::COMP_LT:
        case Token::COMP_GE:
        case Token::COMP_LE:
            return_token();
            relOp = parseRelOp();
            simpleExprB = parseSimpleExprA();
            return std::make_unique<Expression>(std::move(simpleExprA), std::move(relOp), std::move(simpleExprB));
        default:
            return_token();
            return std::make_unique<Expression>(std::move(simpleExprA));
    }
}

std::unique_ptr<Writable> SyntaticAnalyzer::parseWritable() {
    Token* token = get_next_token();

    switch (token->token_type) {
        case Token::CONST_STRING:
            return std::make_unique<Writable>((TokenConstString*) token);
        case Token::ID:
        case Token::OPEN_BRACES:
        case Token::NOT:
        case Token::SUB:        
        case Token::CONST_INT:
        case Token::CONST_FLOAT:
        case Token::CONST_CHAR:
            return_token();
            return std::make_unique<Writable>(parseSimpleExprA());
        default:
            print_error();
            eat_until(Writable::follow);
            return nullptr;
    }
}

std::unique_ptr<WriteStmt> SyntaticAnalyzer::parseWriteStmt() {
    Token* token = get_next_token();

    if (token->token_type != Token::WRITE) {
        print_error();
        eat_until(WriteStmt::follow);
        return nullptr;
    }

    token = get_next_token();
    if (token->token_type != Token::OPEN_BRACES) {
        print_error();
        eat_until(WriteStmt::follow);
        return nullptr;
    }

    std::unique_ptr<WriteStmt> writeStmt = std::make_unique<WriteStmt>(parseWritable());

    token = get_next_token();
    if (token->token_type != Token::CLOSE_BRACES) {
        print_error();
        eat_until(WriteStmt::follow);
    }

    return writeStmt;
}

std::unique_ptr<ReadStmt> SyntaticAnalyzer::parseReadStmt() {
    Token* token = get_next_token();

    if (token->token_type != Token::READ) {
        print_error();
        eat_until(ReadStmt::follow);
        return nullptr;
    }

    token = get_next_token();
    if (token->token_type != Token::OPEN_BRACES) {
        print_error();
        eat_until(ReadStmt::follow);
        return nullptr;
    }

    token = get_next_token();
    if (token->token_type != Token::ID) {
        print_error();
        eat_until(ReadStmt::follow);
        return nullptr;
    }

    std::unique_ptr<ReadStmt> readStmt = std::make_unique<ReadStmt>((TokenId*) token);

    token = get_next_token();
    if (token->token_type != Token::CLOSE_BRACES) {
        print_error();
        eat_until(ReadStmt::follow);
        return nullptr;
    }

    return readStmt;
}

std::unique_ptr<StmtPrefix> SyntaticAnalyzer::parseStmtPrefix() {
    Token* token = get_next_token();

    if (token->token_type != Token::WHILE) {
        print_error();
        eat_until(StmtPrefix::follow);
        return nullptr;
    }

    std::unique_ptr<Condition> condition = parseCondition();

    token = get_next_token();
    if (token->token_type != Token::DO) {
        print_error();
        eat_until(StmtPrefix::follow);
        return nullptr;
    }

    return std::make_unique<StmtPrefix>(std::move(condition));
}

std::unique_ptr<WhileStmt> SyntaticAnalyzer::parseWhileStmt() {
    std::unique_ptr<StmtPrefix> stmtPrefix = parseStmtPrefix();
    std::unique_ptr<StmtList> stmtList = parseStmtList();

    Token* token = get_next_token();
    if (token->token_type != Token::END) {
        print_error();
        eat_until(WhileStmt::follow);
        return nullptr;
    }

    return std::make_unique<WhileStmt>(std::move(stmtPrefix), std::move(stmtList));
}

std::unique_ptr<StmtSuffix> SyntaticAnalyzer::parseStmtSuffix() {
    Token* token = get_next_token();

    if (token->token_type != Token::UNTIL) {
        print_error();
        eat_until(StmtSuffix::follow);
        return nullptr;
    }

    return std::make_unique<StmtSuffix>(parseCondition());
}

std::unique_ptr<RepeatStmt> SyntaticAnalyzer::parseRepeatStmt() {
    Token* token = get_next_token();

    if (token->token_type != Token::REPEAT) {
        print_error();
        eat_until(RepeatStmt::follow);
        return nullptr;
    }

    std::unique_ptr<StmtList> stmtList = parseStmtList();
    std::unique_ptr<StmtSuffix> stmtSuffix = parseStmtSuffix();
    
    return std::make_unique<RepeatStmt>(std::move(stmtList), std::move(stmtSuffix));
}

std::unique_ptr<Condition> SyntaticAnalyzer::parseCondition() {
    return std::make_unique<Condition>(parseExpression());
}

std::unique_ptr<IfStmt> SyntaticAnalyzer::parseIfStmt() {
    Token* token = get_next_token();

    if (token->token_type != Token::IF) {
        print_error();
        eat_until(IfStmt::follow);
        return nullptr;
    }

    std::unique_ptr<Condition> condition = parseCondition();

    token = get_next_token();
    if (token->token_type != Token::THEN) {
        print_error();
        eat_until(IfStmt::follow);
        return nullptr;
    }

    std::unique_ptr<StmtList> ifStmt = parseStmtList();
    std::unique_ptr<StmtList> elseStmt;

    token = get_next_token();
    switch (token->token_type) {
        case Token::END:
            return std::make_unique<IfStmt>(std::move(condition), std::move(ifStmt));
        case Token::ELSE:
            elseStmt = parseStmtList();
            token = get_next_token();
            if (token->token_type != Token::END) {
                print_error();
                eat_until(IfStmt::follow);
                return nullptr;
            }
            return std::make_unique<IfStmt>(std::move(condition), std::move(ifStmt), std::move(elseStmt));        
        default:
            print_error();
            eat_until(IfStmt::follow);
            return nullptr;
    }
}

std::unique_ptr<AssignStmt> SyntaticAnalyzer::parseAssignStmt() {
    Token* token = get_next_token();


    if (token->token_type != Token::ID) {
        print_error();
        eat_until(AssignStmt::follow);
        return nullptr;
    }

    TokenId* id= (TokenId*) token;

    token = get_next_token();
    if (token->token_type != Token::ASSIGN) {
        print_error();
        eat_until(AssignStmt::follow);
        return nullptr;
    }
   
    return std::make_unique<AssignStmt>(id, parseSimpleExprA());
}

std::unique_ptr<Stmt> SyntaticAnalyzer::parseStmt() {
    Token* token = get_next_token();
    return_token();
    switch (token->token_type) {
        case Token::ID:
            return std::make_unique<Stmt>(parseAssignStmt());
        case Token::READ:
            return std::make_unique<Stmt>(parseReadStmt());
        case Token::WRITE:
            return std::make_unique<Stmt>(parseWriteStmt());
        case Token::IF:
            return std::make_unique<Stmt>(parseIfStmt());
        case Token::WHILE:
            return std::make_unique<Stmt>(parseWhileStmt());
        case Token::REPEAT:
            return std::make_unique<Stmt>(parseRepeatStmt());        
        default:
            print_error();
            eat_until(Stmt::follow);
            return nullptr;
    }
}

std::unique_ptr<StmtList> SyntaticAnalyzer::parseStmtList() {
    std::list<std::unique_ptr<Stmt>> stmtList;

    stmtList.push_back(parseStmt());

    Token* token = get_next_token();
    while (token->token_type == Token::SEMICOLON) {
        stmtList.push_back(parseStmt());
        token = get_next_token();
    }
    return_token();

    return std::make_unique<StmtList>(std::move(stmtList));
}

std::unique_ptr<Type> SyntaticAnalyzer::parseType() {
    Token* token = get_next_token();

    switch (token->token_type) {
        case Token::INT:
            return std::make_unique<Type>(Type::INT);
        case Token::FLOAT:
            return std::make_unique<Type>(Type::FLOAT);
        case Token::CHAR:
            return std::make_unique<Type>(Type::CHAR);
        default:
            print_error();
            eat_until(Type::follow);
            return nullptr;
    }
}

std::unique_ptr<IdentList> SyntaticAnalyzer::parseIdentList() {
    std::list<TokenId*> identList;

    Token* token = get_next_token();

    if (token->token_type != Token::ID) {
        print_error();
        eat_until(IdentList::follow);
        return nullptr;
    }
    
    identList.push_back((TokenId*) token);

    token = get_next_token();
    while (token->token_type == Token::COMMA) {
        token = get_next_token();
        if (token->token_type != Token::ID) {
            print_error();
            eat_until(IdentList::follow);
            return nullptr;
        }
        identList.push_back((TokenId*) token);
        token = get_next_token();
    }
    return_token();

    return std::make_unique<IdentList>(std::move(identList));
}

std::unique_ptr<Decl> SyntaticAnalyzer::parseDecl() {
    std::unique_ptr<Type> type = parseType();
    std::unique_ptr<IdentList> identList = parseIdentList();

    return std::make_unique<Decl>(std::move(type), std::move(identList));
}

std::unique_ptr<DeclList> SyntaticAnalyzer::parseDeclList() {
    std::list<std::unique_ptr<Decl>> declList;

    Token* token = get_next_token();
    if (token->token_type != Token::DECLARE) {
        print_error();
        eat_until(DeclList::follow);
        return nullptr;
    }

    declList.push_back(parseDecl());
    
    token = get_next_token();
    if (token->token_type != Token::SEMICOLON) {
        print_error();
        eat_until(DeclList::follow);
        return nullptr;
    }

    token = get_next_token();
    while (true) {
        switch (token->token_type) {
            case Token::INT:
            case Token::FLOAT:
            case Token::CHAR:
                return_token();
                declList.push_back(parseDecl());
                break;
            default:
                return_token();
                return std::make_unique<DeclList>(std::move(declList));
        }

        token = get_next_token();
        if (token->token_type != Token::SEMICOLON) {
            print_error();
            eat_until(DeclList::follow);
            return nullptr;
        }

        token = get_next_token();
    }
}

std::unique_ptr<Body> SyntaticAnalyzer::parseBody() {
    Token* token = get_next_token();


    if (token->token_type != Token::BEGIN) {
        return_token();
        std::unique_ptr<DeclList> declList = parseDeclList();
        
        token = get_next_token();
        if (token->token_type != Token::BEGIN) {
            print_error();
            eat_until(Body::follow);
            return nullptr;
        }

        std::unique_ptr<StmtList> stmtList = parseStmtList();
        token = get_next_token();
        if (token->token_type != Token::END) {
            print_error();
            eat_until(Body::follow);
            return nullptr;
        }

        return std::make_unique<Body>(std::move(declList), std::move(stmtList));
    }

    std::unique_ptr<StmtList> stmtList = parseStmtList();
    token = get_next_token();
    if (token->token_type != Token::END) {
        print_error();
        eat_until(Body::follow);
        return nullptr;
    }

    return std::make_unique<Body>(std::move(stmtList));
}

std::unique_ptr<Program> SyntaticAnalyzer::parseProgram() {
    Token* token = get_next_token();

    if (token->token_type != Token::ROUTINE) {
        print_error();
        eat_until(Program::follow);
        return nullptr;
    }

    return std::make_unique<Program>(parseBody());
}