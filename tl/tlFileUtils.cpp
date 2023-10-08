#include "tlFileUtils.h"
#include "tlStream.h"
#include "tlLog.h"
#include "tlInternational.h"
#include "tlEnv.h"
#include <cctype>

//(_MSC_VER)
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <io.h>
#  include <Windows.h>

// tl命名空间
namespace tl
{
    // 操作系统标识
    enum {
         OS_Auto, 
         OS_Windows, 
         OS_Linux 
         } 
         s_mode = OS_Auto;

    // 判断操作系统
    static bool is_win ()
    {
        if (s_mode == OS_Windows) 
        {
            return true;
        } else if (s_mode == OS_Linux) 
        {
            return false;
        } else 
        {
        #if defined(_WIN32)
            return true;
        #else
            return false;
        #endif
        }
    }
    
    // 设置换行符
    /*
        CRLF : windows   返回 "\r\n"
        LF   : linux     返回  "\n"
    */
    const char *line_separator ()
    {
        return is_win () ? "\r\n" : "\n";
    }

    /*
        判断路径是否为磁盘符号(C:,D:)

        根据字符串参数是否由1个字母加上:组成
        例如  A: 返回true
            
    */
    static bool is_drive (const std::string &part)
    {
        return is_win () && (part.size () == 2 && isalpha (part[0]) && part[1] == ':');
    }

    /*
        windows操作系统中 设置路径为统一
    */
    static std::string normalized_part (const std::string &part)
    {
        if (! is_win ()) {
            return part;
        }

        std::string p;
        p.reserve (part.size ());
        const char *cp = part.c_str ();
        while (*cp == '\\' || *cp == '/') {
            p += '\\';
            ++cp;
        }
        p += cp;
        return p;
    }

    /*
        E:\\installer\\key

        E:\installer\key
        保留 '\' '//'
    */
    static std::string trimmed_part (const std::string &part)
    {
        const char *cp = part.c_str ();

        if (is_win ()) {
            while (*cp == '\\' || *cp == '/') {
            ++cp;
            }
        } else {
            while (*cp == '/') {
            ++cp;
            }
        }
        return std::string (cp);
    }

    /*
        是否包含分隔符
    */
    static bool is_part_with_separator (const std::string &part)
    {
        const char *cp = part.c_str ();
        if (is_win ()) {
            return (*cp == '\\' || *cp == '/');
        } else {
            return (*cp == '/');
        }
    }

    /*
    此函数将路径拆分为其组件
    在 Windows 上，第一个组成部分可能是驱动器前缀（“C:”）或
    UNC 服务器名称（“\\服务器”）。
    组件将保留其路径分隔符，因此连接各个部分将呈现原始路径。 如果路径以分隔符（如“C:\”或“/home/user/”）终止，则会添加尾随空元素。
    这个想法是最后一个元素是文件名部分。 如果“keep_last”为真，则即使最后部分为空，也会保留它。 这样，像“/hello/”这样的路径就变成了“/hello”+“/”。
    */
    std::vector<std::string> split_path (const std::string &p, bool keep_last)
    {
        std::vector<std::string> parts;

        bool first = true;

        if (is_win ()) {

            const char *cp = p.c_str ();
            if (*cp && isalpha (*cp) && cp[1] == ':') 
            {

                //  磁盘名称 (C:,D:)
                parts.push_back (std::string ());   //添加空字符 意义暂不明
                parts.back () += toupper (*cp);
                parts.back () += ":";

                cp += 2;

            } else if ((*cp == '\\' && cp[1] == '\\') || (*cp == '/' && cp[1] == '/')) 
            {

                //  UNC server name
                const char *cp0 = cp;
                cp += 2;
                while (*cp && *cp != '\\' && *cp != '/') {
                    ++cp;
                }
                parts.push_back (tl::normalized_part (std::string (cp0, 0, cp - cp0)));

            } else if ((*cp == '\\' || *cp == '/') && cp[1] && isalpha (cp[1]) && cp[2] == ':') {

            //  drive name in the form "/c:" or "\c:"
            parts.push_back (std::string ());
            parts.back () += toupper (cp[1]);
            parts.back () += ":";

            cp += 3;

            }

            while (*cp) {

            const char *cp0 = cp;
            bool any = false;
            while (*cp && (!any || (*cp != '\\' && *cp != '/'))) {
                if (*cp != '\\' && *cp != '/') {
                any = true;
                } else {
                cp0 = cp;
                }
                ++cp;
            }

            if (any || first || keep_last) {
                first = false;
                parts.push_back (tl::normalized_part (std::string (cp0, 0, cp - cp0)));
            }

            }

        } else {

        const char *cp = p.c_str ();
        while (*cp) {

        const char *cp0 = cp;
        bool any = false;
        while (*cp && (!any || *cp != '/')) {
            if (*cp != '/') {
            any = true;
            } else {
            cp0 = cp;
            }
            //  backslash escape
            if (*cp == '\\' && cp[1]) {
            ++cp;
            }
            ++cp;
        }

        if (any || first || keep_last) {
            first = false;
            parts.push_back (std::string (cp0, 0, cp - cp0));
        }

        }

    }

    return parts;
    }

}