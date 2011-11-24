//#include <memory>
//#include <forward_list>
//#include <string>

//#include "pascal_grammar.h"
#include "ast_visitors.h"
//#include "node_traits.h"

namespace pascal_grammar {
    void add_sections(PascalGrammar& g) {

        typedef PascalGrammar::RightAssociative RightAssociative;

        static auto opening_bracket_scan_enum = 
        [&g](PrattParser<PNode>& p) -> PNode {
            PascalGrammar::behaviour_guard<RightAssociative> guard(*(g.comma), 
                [&g](PNode x, PNode y) {
                    return ListVisitor<IdentifierNode>(x, y, &g, "identifier")
                           .get_expression();
                });
           
            PNode x = p.parse(0);
            g.advance(")", "expected closing ')'");

            if (!node_traits::is_list_of<IdentifierNode>(x))
                g.error("expected list of identifiers");

            return std::make_shared<EnumeratedTypeNode>(x);
        };

        // Variable declarations
        g.colon = &g.infix(":", 70, 
        [&g](PNode x, PNode y) -> PNode {
            if (!node_traits::is_list_of<IdentifierNode>(x)) 
                g.error("expected identifier list");
            if (!node_traits::is_type(y))
                g.error("expected type name");
            return std::make_shared<VariableDeclNode>(x, y);
        });

       g.var = &g.add_symbol_to_dict("var", 1);
       g.var -> nud = [&g, opening_bracket_scan_enum](PrattParser<PNode>& p) -> PNode {
            PascalGrammar::lbp_guard semicolon_guard(*(g.semicolon), 0);

            PascalGrammar::nud_guard open_bracket_guard(*(g.opening_bracket),
                    opening_bracket_scan_enum);

            PascalGrammar::behaviour_guard<RightAssociative> comma_guard(*(g.comma), 
                [&g](PNode x, PNode y) {
                    return ListVisitor<IdentifierNode>(x, y, &g, "identifier")
                           .get_expression();
                });
            
            std::forward_list<PNode> variable_declarations;

            do {
                PNode x = p.parse(1);
                if (!node_traits::has_type<VariableDeclNode>(x))
                    g.error("expected variable declaration");
                variable_declarations.push_front(x);

                g.advance(";", "expected ';' after variable declaration");

                std::string next = p.next_token_as_string();
                if (next == "begin" || next == "procedure" || next == "function" ||
                    next == "const" || next == "type" || next == "var" || next == "") 
                    break;
            } while (true);

            variable_declarations.reverse();

            return std::make_shared<VariableSectionNode>(
                     std::make_shared<VariableDeclListNode>(
                             std::move(variable_declarations)));
        };

        // Type declarations
       g.add_symbol_to_dict("type", 1)
        .nud = [&g, opening_bracket_scan_enum](PrattParser<PNode>& p) -> PNode {
            PascalGrammar::lbp_guard semicolon_guard(*(g.semicolon), 0);
            PascalGrammar::lbp_guard equal_sign_guard(*(g.sign_eq), 0);

            PascalGrammar::nud_guard open_bracket_guard(*(g.opening_bracket),
                    opening_bracket_scan_enum);
            
            std::forward_list<PNode> type_definitions;

            do {
                PNode id = p.parse(1);
                if (!node_traits::has_type<IdentifierNode>(id))
                    g.error("expected identifier as a type name");

                g.advance("=", "expected '=' after type name");

                PNode type = p.parse(1);
                if (!node_traits::is_type(type))
                    g.error("expected type definition after '='");
                
                g.advance(";", "expected ';' after type definition");

                type_definitions.push_front(std::make_shared<TypeDefinitionNode>(id, type));
                std::string next = p.next_token_as_string();

                if (next == "begin" || next == "procedure" || next == "function" ||
                    next == "const" || next == "type" || next == "var" || next == "") 
                    break;
            } while (true);

            type_definitions.reverse();
            return std::make_shared<TypeSectionNode>(std::move(type_definitions));
        };

        // Constant definitions
       g.add_symbol_to_dict("const", 1)
        .nud = [&g](PrattParser<PNode>& p) -> PNode {
            PascalGrammar::lbp_guard semicolon_guard(*(g.semicolon), 0);
            PascalGrammar::lbp_guard equal_sign_guard(*(g.sign_eq), 0);
            
            std::forward_list<PNode> const_defs;
            do {
                PNode id = p.parse(0);

                if (!node_traits::has_type<IdentifierNode>(id))
                    g.error("expected identifier in constant definition");

                g.advance("=", "expected '=' after identifier");

                PNode constant = p.parse(0);

                if (!node_traits::is_convertible_to<ConstantNode>(constant))
                    g.error("expected constant after '='");
                else
                    constant = node::convert_to<ConstantNode>(constant);

                const_defs.push_front(
                        std::make_shared<ConstDefinitionNode>(id, constant));

                g.advance(";", "expected ';' after constant definition");

                std::string next = p.next_token_as_string();
                if (next == "begin" || next == "procedure" || next == "function" ||
                    next == "const" || next == "type" || next == "var" || next == "") 
                    break;
            } while (true);
            const_defs.reverse();
            return std::make_shared<ConstSectionNode>(std::move(const_defs));
        };
    }
} // namespace pascal_grammar
