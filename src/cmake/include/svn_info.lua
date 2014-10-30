INDENT = "    "

function escape(str)
    str = string.gsub(str, "\\", "\\\\")
    str = string.gsub(str, "\"", "\\\"")
    str = string.gsub(str, "\r", "\\r")
    str = string.gsub(str, "\n", "\\n")
    return str
end

function file_exists(filename)
    local f = io.open(filename, "r")
    if (f == nil) then
        return 0
    else
        io.close(f)
        return 1
    end
end

function preadline(command)
    local h = io.popen(command, 'r')
    if (h ~= nil) then
        local data = h:read()
        io.close(h)
        return data
    end
    return nil
end

function host_is_windows()
    local windows_marker = os.getenv("ComSpec")
    if (windows_marker == nil) then
        return false
    else
        return true
    end
end

-- Returns data table with the url, rev, lastchg and author fields.
-- The 'valid' field indicates success.
function get_svn_data_table(fname)
    local data_table = {author='', url='', rev='-1', lastchg='-1'}
    local svn = nil
    if (host_is_windows()) then
        svn = io.popen('svn info ' .. fname)
    else
        svn = io.popen('LANG=C svn info ' .. fname)
    end
    if (svn == nil) then return data_table end

    for line in svn:lines() do
        if (string.find(line, '^URL: ') ~= nil) then
            data_table['url'] = string.sub(line, 6, -1)
        end
        if (string.find(line, '^Revision: ') ~= nil) then
            data_table['rev'] = string.sub(line, 11, -1)
            data_table.valid = true
        end
        if (string.find(line, '^Last Changed Rev: ') ~= nil) then
            data_table['lastchg'] = string.sub(line, 19, -1)
        end
        if (string.find(line, '^Last Changed Author: ') ~= nil) then
            data_table['author'] = string.sub(line, 22, -1)
        end
    end

    svn:close()
    return data_table
end

-- returns information string on success, or nil on failure
function get_svn_data(fname)
    local data = get_svn_data_table(fname)
    if not data.valid then return nil end
    return "Svn info:\\n" ..
        INDENT .. "URL: " .. escape(data['url']) .. "\\n" ..
        INDENT .. "Last Changed Rev: " .. escape(data['lastchg']) .. "\\n" ..
        INDENT .. "Last Changed Author: " .. escape(data['author']) .. "\\n"
end

function get_git_data(src_dir)
    if (not file_exists(src_dir..".git/config")) then
        return nil
    end

    local commit = preadline('git --git-dir "'..src_dir..'/.git" rev-parse HEAD')
    if (commit == nil or string.len(commit) ~= 40) then
        return nil
    end

    local out_data = "git info:\\n"
    out_data = out_data..INDENT.."Commit: "..commit.."\\n"

    local author = preadline('git --git-dir "'..src_dir..'/.git" log -1 --format="format:%an <%ae>" '..commit)
    if (author ~= nil) then
        out_data = out_data..INDENT.."Author: "..escape(author).."\\n"
    end

    local subj = preadline('git --git-dir "'..src_dir..'/.git" log -1 --format="format:%s" '..commit)
    if (subj ~= nil) then
        out_data = out_data..INDENT.."Summary: "..escape(subj).."\\n"
    end

    local body = preadline('git --git-dir "'..src_dir..'/.git" log -1 --grep="^git-svn-id: " --format="format:%b" ')
    if (body ~= nil) then
        local url, rev, id = string.match(body, 'git%-svn%-id: (.*)@(%d*) (.*)')
        if (url ~= nil) then
            out_data = out_data.."\\ngit-svn info:\\n"
            out_data = out_data..INDENT.."URL: "..url.."\\n"
            out_data = out_data..INDENT.."Revision: "..rev.."\\n"
        end
    end

    return out_data
end

function get_scm_data(src_dir)
    local result = ""

    result = get_svn_data(src_dir)
    if (result ~= nil) then
        return result
    end

    result = get_git_data(src_dir)
    if (result ~= nil) then
        return result
    end

    return "Svn info:\\n"..INDENT.."no svn info\\n"
end

function get_local_cmake_info(src_dir, build_dir)
    -- List of directories checked by INIT_CONFIG macro
    local dirs = { src_dir.."/..", src_dir, build_dir.."/..", build_dir }

    local local_cmake = ""
    for i=1,4 do
        local f = io.open(dirs[i].."/local.cmake", "r")
        if (f ~= nil) then
            for line in f:lines() do
                local_cmake = local_cmake..INDENT..INDENT..escape(line).."\\n"
            end
            io.close(f)
        end
    end

    if local_cmake == "" then
        return ""
    else
        return INDENT.."local.cmake:\\n"..local_cmake
    end
end

function get_host_info()
    local cc = nil
    if (not host_is_windows()) then
        cc = io.popen('uname -a')
    else
        cc = io.popen('VER')
    end

    if (cc == nil) then return nil end
    local ver = ""
    for line in cc:lines() do
        ver = ver..INDENT..INDENT..escape(line).."\\n"
    end
    cc:close()

    return ver
end

function get_user()
    local user = os.getenv("USER")
    if (user == nil) then
        user = os.getenv("USERNAME")
    end
    if (user == nil) then
        user = "Unknown user"
    end
    
    return user
end

function get_hostname()
    local hostname = os.getenv("HOSTNAME")
    if (hostname == nil) then
        hostname = os.getenv("COMPUTERNAME")
    end
--    if (hostname == nil) then
--        hostname = os.hostname()
--    end
    if (hostname == nil) then
        hostname = "No host information"
    end
    
    return hostname
end

function get_other_data(src_dir, build_dir)
    local out_data = "Other info:\\n"

    local user = get_user()

    local hostname = get_hostname()

    local build_date = os.date()

    out_data = out_data..INDENT.."Build by: "..user.."\\n"
    out_data = out_data..INDENT.."Top src dir: "..src_dir.."\\n"
    if (build_dir ~= nil) then
        out_data = out_data..INDENT.."Top build dir: "..build_dir.."\\n"
    end
    out_data = out_data..INDENT.."Build date: "..build_date.."\\n"
    out_data = out_data..INDENT.."Hostname: "..hostname.."\\n"
    out_data = out_data..INDENT.."Host information: \\n"..get_host_info().."\\n"

    out_data = out_data..get_local_cmake_info(src_dir, build_dir)

    return out_data
end

function get_compiler_version(compiler)
    local cc = nil
    if (host_is_windows()) then
        cc = io.popen('"' .. compiler .. '" 2>&1')
    else
        cc = io.popen(compiler .. ' --version')
    end

    if (cc == nil) then return nil end
    local ver = ""
    for line in cc:lines() do
        if line == "" then
            break -- skip "usage: cl ..." on Windows
        end
        ver = ver..INDENT..INDENT..escape(line).."\\n"
    end
    cc:close()

    return ver
end

function get_build_info(compiler, fname)
    local out_data = "Build info:\\n"
    out_data = out_data..INDENT.."Compiler: "..compiler.."\\n"

    out_data = out_data..INDENT.."Compiler version: \\n"..get_compiler_version(compiler)

    local flags_info = INDENT.."Compile flags:"..fname.." no flags info\\n"
    local f = io.open(fname, "r")
    if (f ~= nil) then
        for line in f:lines() do
            local a, b = line:find("CXX_FLAGS")
            if a ~= nil and a == 1 then
                flags_info = INDENT.."Compile flags: "..escape(line:sub(13))
                break
            end
        end
        io.close(f)
    end

    return out_data..flags_info
end

local scm_data = get_scm_data(arg[1])
local svn_data_table = get_svn_data_table(arg[1])

print([[#pragma once

#define PROGRAM_VERSION "]]..scm_data.."\\n"..get_other_data(arg[1], arg[4]).."\\n"..get_build_info(arg[2], arg[3])..[["
#define SCM_DATA "]]..scm_data..[["
#define ARCADIA_SOURCE_PATH "]]..arg[1]..[["
#define ARCADIA_SOURCE_URL "]]..svn_data_table['url']..[["
#define ARCADIA_SOURCE_REVISION "]]..svn_data_table['rev']..[["
#define ARCADIA_SOURCE_LAST_CHANGE "]]..svn_data_table['lastchg']..[["
#define ARCADIA_SOURCE_LAST_AUTHOR "]]..svn_data_table['author']..[["
#define BUILD_USER "]]..get_user()..[["
#define BUILD_HOST "]]..get_hostname()..[["
#define BUILD_DATE "]]..os.date()..[["

]])
