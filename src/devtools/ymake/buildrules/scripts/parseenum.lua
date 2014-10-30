--require "stdlib"

--[[
Allowed syntax is like this:

enum ESuperEnum {
    SV_ONE = 10 /* "one" */,
    SV_TWO      /* "two" */
}

so enum string values will be "one" and "two" respectively,
or just without pre-defined string values, like this:

enum EEasyEnum {
    EV_ONE = 1,
    EV_TWO
}
so enum string values will be "EV_ONE" and "EV_TWO" respectively.

Restrictions: All numeric values after '=' sign should be DECIMAL,
no hexadeciamal or macro values allowed yet. Negative values
are SUPPORTED, though.
]]--

verbose = false

function msg(s)
    if verbose then
        print(s)
    end
end

function get_input_file_name()
    if arg[1] == nul then
        error("Input file argument is not specified")
    end

    return arg[1]
end

function get_output_file_name()
    if arg[2] == nul then
        error("Output file argument is not specified")
    end

    return arg[2]
end

function strip(s)
    return string.gsub(s, "^%s*(.-)%s*$", "%1")
end

function is_empty(s)
    return s:len() == 0
end

function find_last_of(s, subStr)
    local lastPos = -1

    while true do
        local pos = string.find(s, subStr, lastPos + 1)

        if not (pos == nil) then
            lastPos = pos
        elseif lastPos == -1 then
            return nil
        else
            return lastPos
        end
    end
end

function cut_line_at(line, mark)
    local endPos = string.find(line, mark)
    if endPos == 1 then
        return ""
    end
    if not (endPos == nil) then
        line = string.sub(line, 1, endPos - 1)
    end
    line = strip(line)
    return line;
end

function get_file_name(path)
    local pos = find_last_of(path, "/")

    if pos == nil then
        return path
    end

    return string.sub(path, pos + 1)
end

function check_name_line(line)
    line = cut_line_at(line, "//")
    if is_empty(line) then
        return line
    end

    local spacePos = string.find(line, " ")
    if spacePos == nil then
        return ""
    end

    local enum = string.sub(line, 1, spacePos - 1)
    if (enum ~= "enum") then
        return ""
    end

    line = string.sub(line, spacePos + 1, #line)

    -- Skip class/struct keyword
    local enum_class = line
    enum_class = cut_line_at(enum_class, " ");
    if enum_class == "class" or enum_class == "struct" then
        local skipPos = string.find(line, enum_class)
        line = string.sub(line, skipPos + string.len(enum_class) + 1, #line)
    end

    -- Find optional underlying type specification
    local cut1 = line
    cut1 = cut_line_at(cut1, " ");
    local cut2 = line
    cut2 = cut_line_at(cut2, ":");
    if string.len(cut1) < string.len(cut2) then
        line = cut1
    else
        line = cut2
    end

    if line == "{" then
       return ""
    end

    return line
end

function count_braces(line)
    local braces = 0
    for b in line:gmatch("{") do
        braces = braces + 1
    end
    for b in line:gmatch("}") do
        braces = braces - 1
    end
    return braces
end

function check_scope(line, scope)
    line = cut_line_at(line, "//")
    if is_empty(line) then
        return scope
    end
    local decl = line:match("%w+")
    local last = table.maxn(scope)
    if decl ~= nil and (decl == "namespace" or decl == "struct" or decl == "class") then
        if next(scope) and scope[last][1] == 0 then
            msg("leave scope: " .. scope[last][0])
            table.remove(scope, last);
        end

        local name = line:match("%w+%s+(%w+)")
        if not line:match(";$") then
            local n = count_braces(line) -- fixme: count after declaration only
            table.insert(scope, { [0] = name, [1] = n } )
            msg("enter scope: " .. name)
        end
    else
        if next(scope) == nil then -- empty
            return scope
        end
        n = count_braces(line)
        while (n ~= 0) do
            local sum = scope[last][1] + n
            if (sum > 0) then
                scope[last][1] = sum
                n = 0
            else
                msg("leave scope: " .. scope[last][0])
                table.remove(scope, last)
                n = sum
            end
        end
    end
    return scope
end

function scope_name(scope)
    local name = ""
    for i, v in ipairs(scope) do
        if v[1] > 0 then
            name = name .. v[0] .. "::"
        end
    end
    return name
end

function get_field_data(line)
    line = cut_line_at(line, "//")
    line = cut_line_at(line, "}")
    return line
end

function write_header(outFile, name)
    outFile:write("// This file was auto-generated. Do not edit!!!\n")
    outFile:write("#include \"" .. name .. "\"\n")
    outFile:write("#include <util/generic/typetraits.h>\n")
    outFile:write("#include <util/generic/singleton.h>\n")
    outFile:write("#include <util/generic/stroka.h>\n")
    outFile:write("#include <util/generic/map.h>\n")
    outFile:write("#include <util/stream/output.h>\n\n")
end

-- generic length operator counts only positive array indices, that's not good
function table_size(fields)
    local count = 0
    for k, v in pairs(fields) do
        count = count + 1
    end
    return count
end

function write_enum(outFile, name, fields, hasEq)
    local count = table_size(fields)
    outFile:write("// io for " .. name .. "\n")
    local nsname = "N" .. name:gsub("::", "") .. "Private"
    outFile:write("namespace { namespace " .. nsname .. " {\n")
    outFile:write("    class TNameBufs {\n")
    outFile:write("    private:\n")
    if hasEq == true then
        outFile:write("        ymap<int, Stroka> Names;\n")
    else
        outFile:write("        Stroka Names[" .. count .. "];\n")
    end
    outFile:write("        Stroka AllNames;\n")
    outFile:write("    public:\n")
    outFile:write("        TNameBufs() {\n")
    for k, v in pairs(fields) do
        outFile:write("            Names[" .. k .. "] = \"" .. v .. "\";\n")
    end
    outFile:write("\n")
    if count > 0 then
        if hasEq == true then
            outFile:write("            for (ymap<int, Stroka>::const_iterator i = Names.begin(); i != Names.end(); ++i)\n")
            outFile:write("                AllNames += \"'\" + i->second + \"', \";\n")
        else
            outFile:write("            for (int i = 0; i < " .. count .. "; ++i)\n")
            outFile:write("                AllNames += \"'\" + Names[i] + \"', \";\n")
        end
        outFile:write("                AllNames = AllNames.substr(0, AllNames.size() - 2);\n")
    end
    outFile:write("        }\n\n")
    -- ToString
    outFile:write("        const Stroka& ToString(" .. name .. " i) const {\n")
    if hasEq == true then
        outFile:write("            ymap<int, Stroka>::const_iterator j = Names.find((int)i);\n")
        outFile:write("            if (j == Names.end())\n")
        outFile:write("                ythrow yexception() << \"undefined value \" << (int)i << \" in " .. name .. "\";\n")
        outFile:write("            return j->second;\n")
    else
        outFile:write("            YASSERT((int)i >= 0 && (int)i < " .. count .. ");\n")
        outFile:write("            return Names[(int)i];\n")
    end
    outFile:write("        }\n\n")

    -- bool FromString(const TStringBuf& name, <EnumType>& ret)
    outFile:write("        bool FromString(const TStringBuf& name, " .. name .. "& ret) const {\n")
    if hasEq == true then
        outFile:write("            for (ymap<int, Stroka>::const_iterator i = Names.begin(); i != Names.end(); ++i) {\n")
        outFile:write("                if (i->second == name) {\n")
        outFile:write("                    ret = (" .. name .. ")i->first;\n")
    else
        outFile:write("            for (int i = 0; i < " .. count .. "; ++i) {\n")
        outFile:write("                if (Names[i] == name) {\n")
        outFile:write("                    ret = (" .. name .. ")i;\n")
    end
    outFile:write("                    return true;\n")
    outFile:write("                }\n")
    outFile:write("            }\n")
    outFile:write("            return false;\n")
    outFile:write("        }\n\n")

    -- <EnumType> FromString(const TStringBuf& name)
    outFile:write("        " .. name .. " FromString(const TStringBuf& name) const {\n")
    outFile:write("            " .. name .. " ret = " .. name .. "(0);\n")
    outFile:write("            if (FromString(name, ret))\n")
    outFile:write("                return ret;\n")
    outFile:write("            ythrow yexception() << \"Key '\" << name << \"' not found in enum. Valid options are: \" <<\n")
    outFile:write("                AllEnumNames() << \". \";\n")
    outFile:write("        }\n\n")

    -- Stroka AllEnumNames()
    outFile:write("        const Stroka& AllEnumNames() const {\n")
    outFile:write("            return AllNames;\n")
    outFile:write("        }\n\n")

    -- Instance
    outFile:write("        static inline const TNameBufs& Instance() {\n")
    outFile:write("            return *Singleton<TNameBufs>();\n")
    outFile:write("        }\n")
    outFile:write("    };\n")
    outFile:write("}}\n\n")

    -- outer ToString
    outFile:write("const Stroka& ToString(" .. name .. " x) {\n")
    outFile:write("    const " .. nsname .. "::TNameBufs& names = " .. nsname .. "::TNameBufs::Instance();\n")
    outFile:write("    return names.ToString(x);\n")
    outFile:write("}\n\n")

    -- outer FromString
    outFile:write("bool FromString(const Stroka& name, " .. name .. "& ret) {\n")
    outFile:write("    const " .. nsname .. "::TNameBufs& names = " .. nsname .. "::TNameBufs::Instance();\n")
    outFile:write("    return names.FromString(name, ret);\n")
    outFile:write("}\n\n")

    -- outer FromString
    outFile:write("bool FromString(const TStringBuf& name, " .. name .. "& ret) {\n")
    outFile:write("    const " .. nsname .. "::TNameBufs& names = " .. nsname .. "::TNameBufs::Instance();\n")
    outFile:write("    return names.FromString(name, ret);\n")
    outFile:write("}\n\n")

    -- outer Out
    outFile:write("template<>\n")
    outFile:write("void Out<" .. name .. ">(TOutputStream& os, TTypeTraits<" .. name .. ">::TFuncParam n) {\n")
    outFile:write("    os << ToString(n);\n")
    outFile:write("}\n\n")

    -- <EnumType>AllNames
    outFile:write("const Stroka& " .. name:gsub("::", "") .. "AllNames() {\n")
    outFile:write("    const " .. nsname .. "::TNameBufs& names = " .. nsname .. "::TNameBufs::Instance();\n")
    outFile:write("    return names.AllEnumNames();\n")
    outFile:write("}\n\n")

    -- <EnumType>FromString
    outFile:write(name .. " " .. name:gsub("::", "") .. "FromString(const Stroka& name) {\n")
    outFile:write("    const " .. nsname .. "::TNameBufs& names = " .. nsname .. "::TNameBufs::Instance();\n")
    outFile:write("    return names.FromString(name);\n")
    outFile:write("}\n\n")
end

function parse_enum_line(line)
    line = strip(line)
    local name = ""
    -- cut first expression
    for x in string.gmatch(line, "[A-Za-z0-9_]+") do
        name = x
        break
    end
    -- cut value in quotes if exists
    for x in string.gmatch(line, "\"([A-Za-z0-9_-]+)\"") do
        name = x
        break
    end
    return name
end

function parse_enum_data(str)
    local fields = {}
    local hasEq = false
    local value = 0

    for line in string.gmatch(str, "[^,]+") do
        local eqPos = string.find(line, "=")
        if eqPos == nil then
            local name = parse_enum_line(line)
            fields[value] = name
        else
            local name = parse_enum_line(line)
            local val = string.sub(line, eqPos + 1, #line)
            value = nil
            for x in string.gmatch(val, "[0-9-]+") do
                value = x
                break
            end
            if value == nil then
                print("Cannot parse enum value '" .. val .. "'")
                os.exit(1)
            end
            fields[value] = name
            hasEq = true
        end
        value = value + 1
    end
    return fields, hasEq
end

function parse(inputFileName, outputFileName, ns)
    local endMarker = "};"

    local inFile = assert(io.open(inputFileName, "r"))
    local outFile = assert(io.open(outputFileName, "wb"))

    local fname = get_file_name(inputFileName)
    write_header(outFile, fname)
    if not (ns == nil) then
        outFile:write("using namespace " .. ns .. ";\n\n")
    end

    local fields = {}
    local enumname
    local hasEq = false
    local value = 0

    local inEnum = false
    local enumStr = ""

    local scope = {}

    for line in inFile:lines() do
        scope = check_scope(line, scope)
        if inEnum == false then
            enumname = check_name_line(line)
            if not is_empty(enumname) then
                inEnum = true
                msg("Process " .. enumname .. "...")
            end
        else
            enumStr = enumStr .. get_field_data(line)
            if string.find(line, endMarker) then
                fields, hasEq = parse_enum_data(enumStr)
                msg("scope: " .. scope_name(scope))
                local qualname = scope_name(scope) .. enumname
                write_enum(outFile, qualname, fields, hasEq)

                fields = {}
                enumname = ""
                hasEq = false
                inEnum = false
                enumStr = ""
            end
        end
    end

    io.close(inFile)
    io.close(outFile)
end


function main()
    for k, v in pairs(arg) do
        if v == '-v' or v == '--verbose' then
            verbose = true
            table.remove(arg, k)
            break
        end
    end

    local inputFileName  = get_input_file_name()
    local outputFileName = get_output_file_name()
    parse(inputFileName, outputFileName, arg[3])
    msg("OK")
end

main()
