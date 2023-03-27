#pragma once
#include "verify.hpp"
#include "command.hpp"
#include "context.hpp"
#include "print.hpp"
#include "shell.hpp"
#include "usage.hpp"

namespace cxe {

    constexpr const char CXE_COMMENT_HEAD[] = "/" "*cxe{";
    constexpr const char CXE_COMMENT_TAIL[] = "}*" "/";

    //--------------------------------------------------------------------------

    bool is_cpp_path(const token_t& t) {
        using namespace scan;
        return suffix(".cpp", t, ignore_case)
            or suffix(".cxx", t, ignore_case)
            or suffix(".c++", t, ignore_case)
            or suffix(".cc",  t, ignore_case);
    }

    bool is_c_path(const token_t& t) {
        using namespace scan;
        return suffix(".c", t);
    }

    bool is_c_cpp_path(const token_t& t) {
        return is_c_path(t) or is_cpp_path(t);
    }

    //--------------------------------------------------------------------------

    class parser {
        const context& ctx;
        const tokens_t _cli_toks;
        const tokens_t _src_toks;

        commands _pre_compile;
        command  _compile;
        commands _post_compile;
        command  _execute_cmd;
        command  _execute_args;
        bool     _should_execute;

        parser(const context& ctx)
        : ctx(ctx)
        , _cli_toks(tokenize_cli_text(ctx.cli_text))
        , _src_toks(tokenize_src_text(ctx.src_text))
        , _pre_compile()
        , _compile()
        , _post_compile()
        , _execute_cmd()
        , _execute_args()
        , _should_execute() { }

    public:

        static buffer<command> parse(const context& ctx) {
            cxe::parser parser(ctx); parser.parse();
            cxe::buffer<command> cmds;
            cmds.reserve(
                parser._pre_compile.size() +
                1 + // parser._compile
                parser._post_compile.size() +
                parser._should_execute);

            for (command* cmd : parser._pre_compile) {
                cmds.emplace_back(std::move(*cmd));
            }

            cmds.emplace_back(std::move(parser._compile));

            for (command* cmd : parser._post_compile) {
                cmds.emplace_back(std::move(*cmd));
            }
            
            if (parser._should_execute) {
                cmds.emplace_back(std::move(parser._execute_cmd));
            }

            return cmds;
        }

    private:

        using span_t = context::span_t;

        static tokens_t tokenize_cli_text(span_t text) {
            tokens_t toks;
            toks.reserve(text.size() / 8);
            tokenize(toks, text);
            // println("--");
            // for (int i = 0; i < toks.size(); ++i) {
            //     println("_cli_toks[",i,"]: |",toks[i],"|");
            // }
            return toks;
        }

        static tokens_t tokenize_src_text(span_t text) {
            using namespace ::cxe::scan;
            tokens_t toks;
            toks.reserve(text.size() / 8);
            seek(CXE_COMMENT_HEAD, text);
            skip(CXE_COMMENT_HEAD, text);
            chop(CXE_COMMENT_TAIL, text);
            tokenize(toks, text);
            // println("--");
            // for (int i = 0; i < toks.size(); ++i) {
            //     println("_src_toks[",i,"]: |",toks[i],"|");
            // }
            return toks;
        }

        static void tokenize(tokens_t& dst, token_t src) {
            using namespace ::cxe::scan;
            itr_t itr = src.data();
            end_t end = itr + src.size();

            while (itr < end) {
                skip_while(isspace, itr, end);

                end_t ptr = itr;

                // skip comments
                if (skip('#', itr, end) or skip("//", itr, end)) {
                    seek('\n', itr, end);
                    skip('\n', itr, end);
                    continue;
                }

                // tokenize delimiters & operators
                if (skip('{', itr, end) or skip('}', itr, end) or
                    skip('[', itr, end) or skip(']', itr, end) or
                    skip('(', itr, end) or skip(')', itr, end)) {
                    dst.emplace_back(ptr, itr);
                    continue;
                }

                // tokenize operators
                if (skip("&&", itr, end) or skip("||", itr, end)) {
                    dst.emplace_back(ptr, itr);
                    continue;
                }

                istoken::state st {};
                skip_while(istoken{st}, itr, end);

                if (ptr == itr) continue;

                dst.emplace_back(ptr, itr);
            }
        }

        class tokitr {
            tokens_t::const_iterator itr;
            tokens_t::const_iterator const end;
            const token_t nul;

        public:
            tokitr(const tokens_t& toks)
            : itr  (toks.begin())
            , end  (toks.end())
            , nul  (toks.back().data() + toks.back().size(), size_t(0)) {}

            tokitr(const tokitr&) = default;

            explicit operator bool() const { return itr < end; }

            bool advance() { return (itr < end) and (++itr, true); }

            token_t peek() const { return itr < end ? *itr : nul; }

            token_t read() { token_t tok = peek(); advance(); return tok; }
        };

        location at(token_t t) const { return ctx.locate(t); }

        void parse() {
            using namespace ::cxe::scan;
            verify(_cli_toks.size());
            verify(_src_toks.size());

            append(ctx.compiler_path, _compile);

            if (tokitr itr = _cli_toks) {
                itr.advance(); // skip cxe name
                itr.advance(); // skip src path
                while (itr) parse_arg(itr, _compile);
            }

            if (tokitr itr = _src_toks) {
                while (itr) parse_arg(itr, _compile);
            }

            const char* src_itr = ctx.src_path.data();
            const char* src_end = src_itr + ctx.src_path.size();
            while (seek("/",src_itr,src_end) and skip("/",src_itr,src_end));

            append(token_t(src_itr, src_end), _compile);

            if (_should_execute) {
                if (_execute_cmd.empty()) {
                    _execute_cmd.append(token_t("a"));
                }
                for (const char* arg : _execute_args) {
                    _execute_cmd.append(token_t(arg, strlen(arg)));
                }
            }
        }

        void parse_arg(tokitr& itr, command& cmd) {
            using namespace ::cxe::scan;

            verify(itr);
            const token_t t = itr.read();
            verify(t.size());

            if (equals("-help",t) or equals("--help",t))
                return puts(USAGE), exit(1);

            if (equals("-if",t))
                return parse_if(itr, cmd);

            if (equals("--",t)) {
                _should_execute = true;
                while (itr) parse_arg(itr, _execute_args);
                return;
            }

            if (equals("-pre",t)) {
                parse_block(itr, _pre_compile.append());
                return;
            }

            if (equals("-post",t)) {
                parse_block(itr, _post_compile.append());
                return;
            }

            if (&cmd == &_compile) {
                if (prefix("--output",t) or prefix("-o",t)) {
                    parse_output(t, itr, cmd);
                    append(t, cmd);
                    return;
                }
            }

            append(t, cmd);
        }

        void parse_output(const token_t& t, const tokitr& itr, command& cmd) {
            using namespace ::cxe::scan;

            token_t a = t;
            
            if (skip("--output=",a)) {
                // --output=<file>
                if (not _execute_cmd.empty())
                    return error(1,at(t),"redundant output option");

                append(a, _execute_cmd);
                return;
            }

            if (equals("--output",a) or equals("-o",a)) {
                // --output <file> or -o <file>
                if (not itr)
                    return error(1,at(t),"expected output path");

                token_t b = itr.peek();
                append(b, _execute_cmd);
                return;
            }

            if (prefix("-objcmd-",a) or prefix("-object-",a)) {
                return; // red herring
            }

            if (skip("-o",a)) {
                // -o<file>
                if (a.empty())
                    return error(1,at(t),"expected output path");

                append(a, _execute_cmd);
                return;
            }
        }

        void parse_if(tokitr& itr, command& cmd) {
            using namespace ::cxe::scan;

            const token_t a = itr.read();

            if (not equals("(", a)) error(1, at(a), "expected \"(\"");

            const bool cond = evaluate_conditional(itr, cmd);

            cond ? parse_block(itr, cmd) : skip_block(itr);
        }

        // parse tokens within { ... }
        void parse_block(tokitr& itr, command& cmd) {
            using namespace ::cxe::scan;

            const token_t a = itr.read();

            if (not equals("{", a)) error(1,at(a),"expected \"{\"");

            for (;;) {
                if (not itr) error(1,at(itr.peek()), "expected \"}\"");
                if (equals("}",itr.peek())) {
                    itr.advance();
                    return;
                }
                parse_arg(itr, cmd);
            }
        }

        // skip tokens within { ... }
        void skip_block(tokitr& itr) {
            using namespace ::cxe::scan;

            const token_t a = itr.read();

            if (not equals("{",a)) error(1,at(a),"expected \"{\"");

            for (int depth = 1; depth > 0;) {
                if (not itr) error(1,at(itr.peek()),"expected \"}\"");
                const token_t b = itr.read();
                if (equals("{",b)) ++depth;
                else
                if (equals("}",b)) --depth;
            }
        }

        bool evaluate_conditional(tokitr& itr, command& cmd) {
            using namespace ::cxe::scan;

            const token_t a = itr.read();

            if (equals(")", a)) {
                error(1, at(a), "expected conditional expression");
                return false;
            }

            bool result = false;

            const token_t b = itr.read();

            if (equals(")", b)) {
                // -if ( -DRELEASE )
                //      |a--------|
                return find(scan::equals, a, cmd);
            }

            if (equals("[", b)) {
                // -if ( --target= [
                //      |a--------|b|

                const char* const arg = find(scan::prefix, a, cmd);
                // echo(arg);

                const token_t c = itr.read();
                // echo(c);

                if (equals("]", c)) {
                    // -if ( --target= [ ] )
                    //      |a--------|b|c|
                    result = arg != nullptr;
                    goto close_bracket;
                }

                const token_t d = itr.read();
                // echo(d);

                if (equals("]", d)) { 
                    // -if ( --target= [ windows ]
                    //      |a--------|b|c------|d|
                    if (arg) {
                        const size_t len = strlen(arg);
                        verify(len);
                        token_t s(arg,len); // --target=x86_64-pc-windows-msvc
                        skip(a, s);         //          x86_64-pc-windows-msvc
                        result = contains(c, s);
                    }
                    goto close_bracket;
                }

                error(1, at(d), "expected \"]\"");
                return false;
            }

        close_bracket:

            const token_t e = itr.read();

            if (equals("&&", e) or equals("and", e)) {
                // -if ( --target= [ windows ] &&
                //      |a--------|b|c------|d|e-|
                return result and evaluate_conditional(itr, cmd);
            }

            if (equals("||", e) or equals("or", e)) {
                // -if ( --target= [ windows ] ||
                //      |a--------|b|c------|d|e-|
                return result or evaluate_conditional(itr, cmd);
            }

            if (equals(")", e)) {
                // -if ( --target= [ windows ] )
                //      |a--------|b|c------|d|e|
                return result;
            }

            error(1,at(e),"expected \"&&\"/\"and\", \"||\"/\"or\", or \")\"");
            return false;
        }

        using match_t = command::match_t;

        const char* find(match_t match, const token_t& src, command& cmd) {
            using namespace ::cxe::scan;

            if (const char* arg = cmd.find(match, src))
                return arg;

            // resolve implicit --target to effective triple
            if (prefix("--target", src)) {
                buffer<char> buf; buf << "--target=";
                buffer<char> cc;
                cc << ctx.compiler_path << " -print-effective-triple";
                if (const int status = shell::run(buf, cc)) {
                    error(1,at(src),"failed to resolve --target: ",
                        cc.data()," returned ",status);
                }
                return cmd.append(buf);
            }

            return nullptr;
        }

        const char* append(const token_t& src, command& cmd) {
            const buffer<char> dst = resolve(src);
            return cmd.append(dst);
        }

        buffer<char> resolve(const token_t& src) {
            using namespace ::cxe::scan;
            buffer<char> buf; buf << src;

            if (not environment::resolve_variables(buf)) {
                error(1,at(src),"unresolved environment variable");
            }

            token_t tok { buf.data(), buf.size() };

            // resolve explicit --target= to effective triple
            if (prefix("--target=", tok)) {
                buffer<char> buf2; buf2 << "--target=";
                buffer<char> cc;
                cc << ctx.compiler_path << " " << tok;
                cc << " -print-effective-triple";
                if (const int status = shell::run(buf2, cc)) {
                    error(1,at(src),"failed to resolve --target: ",
                        cc.data()," returned ",status);
                }
                buf = std::move(buf2);
            }

            return buf;
        }
    };

} // namespace cxe