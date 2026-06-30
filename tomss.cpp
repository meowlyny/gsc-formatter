// very bad code ik lol
// this tool is meant to combat script inliners (such as tom crowleys one)

#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

enum class ScriptResult
{
    EmptyPath = 0,
    OpenForReadFailed,
    OpenForWriteFailed,
    Success,
};

namespace
{
    void ReplaceAll(std::string& s, const std::string& from, const std::string& to)
    {
        if (from.empty())
            return;

        size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos)
        {
            s.replace(pos, from.length(), to);
            pos += to.length();
        }
    }

    std::string UnshrinkSource(const std::string& src)
    {
        std::string out;
        int indent = 0;
        bool inString = false;
        char stringChar = 0;

        auto appendIndent = [&]()
        {
            for (int i = 0; i < indent; ++i)
                out += "    ";
        };

        size_t i = 0, n = src.size();
        appendIndent();

        auto isSpace = [](char c)
        {
            return c == ' ' || c == '\t';
        };

        while (i < n)
        {
            char c = src[i];

            if (inString)
            {
                out += c;

                if (c == '\\' && i + 1 < n)
                {
                    out += src[i + 1];
                    i += 2;
                    continue;
                }

                if (c == stringChar)
                    inString = false;

                ++i;
                continue;
            }

            if (c == '"' || c == '\'')
            {
                inString = true;
                stringChar = c;
                out += c;
                ++i;
                continue;
            }

            if (isSpace(c))
            {
                if (!out.empty() && out.back() != ' ' && out.back() != '\n')
                    out += ' ';

                while (i + 1 < n && isSpace(src[i + 1]))
                    ++i;

                ++i;
                continue;
            }

            if ((c == '+' || c == '-') && i + 1 < n && src[i + 1] == '=')
            {
                out += ' ';
                out += c;
                out += "= ";
                i += 2;
                continue;
            }

            if (c == '=')
            {
                if (i + 1 < n && src[i + 1] == '=')
                {
                    out += "==";
                    i += 2;
                    continue;
                }

                out += " = ";
                ++i;
                continue;
            }

            if (c == '{')
            {
                while (!out.empty() && out.back() == ' ')
                    out.pop_back();

                if (!out.empty() && out.back() != '\n')
                    out += '\n';

                appendIndent();
                out += "{\n";

                ++indent;
                appendIndent();
                ++i;
                continue;
            }

            if (c == '}')
            {
                while (!out.empty() && out.back() == ' ')
                    out.pop_back();

                if (!out.empty() && out.back() != '\n')
                    out += '\n';

                --indent;

                appendIndent();
                out += "}\n\n";

                appendIndent();
                ++i;
                continue;
            }

            if (c == ';')
            {
                out += ";\n";
                appendIndent();
                ++i;
                continue;
            }

            if (c == ',')
            {
                out += ", ";
                ++i;
                continue;
            }

            out += c;
            ++i;
        }

        std::string cleaned;
        cleaned.reserve(out.size());

        bool lastSpace = false;
        bool lineStart = true;

        for (char c : out)
        {
            if (c == '\n')
            {
                cleaned += c;
                lastSpace = false;
                lineStart = true;
                continue;
            }

            if (c == ' ')
            {
                if (lineStart)
                {
                    cleaned += c;
                    continue;
                }

                if (!lastSpace)
                    cleaned += c;

                lastSpace = true;
                continue;
            }

            lineStart = false;
            lastSpace = false;
            cleaned += c;
        }

        return cleaned;
    }

    ScriptResult FormatScriptFile(const std::wstring& path, std::wstring& outMessage)
    {
        if (path.empty())
        {
            outMessage = L"No file specified.";
            return ScriptResult::EmptyPath;
        }

        std::ifstream in(path, std::ios::binary);
        if (!in)
        {
            outMessage = L"Could not open file for reading.";
            return ScriptResult::OpenForReadFailed;
        }

        std::ostringstream ss;
        ss << in.rdbuf();
        std::string src = ss.str();
        in.close();

        std::string result = UnshrinkSource(src);

        // Backup original
        std::wstring backupPath = path + L".bak";
        DeleteFile(backupPath.c_str());
        MoveFile(path.c_str(), backupPath.c_str());

        std::ofstream out(path, std::ios::binary);
        if (!out)
        {
            MoveFile(backupPath.c_str(), path.c_str());
            outMessage = L"Could not open file for writing.";
            return ScriptResult::OpenForWriteFailed;
        }

        out << result;
        out.close();

        outMessage = L"formatted successfully.";
        return ScriptResult::Success;
    }
}

int wmain(int argc, wchar_t* argv[])
{
    if (argc < 2)
    {
        wprintf(L"usage: <file.gsc>\n");
        return 1;
    }

    int failed = 0;

    for (int i = 1; i < argc; ++i)
    {
        std::wstring message;
        auto result = FormatScriptFile(argv[i], message);
        wprintf(L"%s: %s\n", argv[i], message.c_str());

        if (result != ScriptResult::Success)
            ++failed;
    }

    return failed ? 1 : 0;
}
