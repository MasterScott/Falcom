import Common

def MakeStrDic(psp_jp_files, pc_cn_files, report):

    if len(psp_jp_files) != len(pc_cn_files):
        print('File num not match')
        input()
        return

    dst_dic = dict()

    for fn_jp, fn_cn in zip(psp_jp_files, pc_cn_files):

        if Common.BaseName(fn_jp) != Common.BaseName(fn_cn):
            print('Filename not match: {} {}'.format(fn_jp, fn_cn))
            input()
            exit(1)

        fname = Common.BaseName(fn_cn)
        print('process jp cn: {}'.format(fname))

        jp = open(fn_jp, 'r', encoding='utf16')
        cn = open(fn_cn, 'r', encoding='utf16')

        lines = jp.readlines()
        jp_lines = [line.rstrip('\n') for line in lines if line != '' and line != '\n']
        lines = cn.readlines()
        cn_lines = [line.rstrip('\n') for line in lines if line != '' and line != '\n']

        if len(jp_lines) != len(cn_lines):
            input('Line num not match: {}'.format(Common.BaseName(fn_jp)))
            exit(1)

        for jp_line, cn_line in zip(jp_lines, cn_lines):
            offset, size, jp_text = Common.ParseWQSGLine(jp_line)
            offset, size, cn_text = Common.ParseWQSGLine(cn_line)

            if (not jp_text) and (not cn_text):
                continue

            jp_group = Common.SplitString(jp_text)
            cn_group = Common.SplitString(cn_text)

            if len(jp_group) != len(cn_group):
                log = 'Group not match {}:{}--{}'.format(fname, jp_text, cn_text)
                print(log)
                report.write(log + '\n')
                continue

            for key, value in zip(jp_group, cn_group):
                dst_dic[key] = value

        jp.close()
        cn.close()

    return dst_dic


def IsAlNum(string):
    for ch in string:
        if ord(ch) >= 0x20 and ord(ch) <= 0x7F:
            continue
        else:
            return False
    return True


def ReadVitaGroups(vita_fn_list):

    dst_list = []
    for fn in vita_fn_list:
        fname = Common.BaseName(fn)
        print('process vita txt: {}'.format(fname))
        src = open(fn, 'r', encoding='utf16')

        _lines = src.readlines()
        lines = [line.rstrip('\n') for line in _lines if line != '' and line != '\n']

        for line in lines:
            offset, size, text = Common.ParseWQSGLine(line)

            if not text:
                continue

            group = Common.SplitString(text)
            for string in group:

                if IsAlNum(string):
                    continue

                if len(string) >= 2 and \
                        ((IsAlNum(string[0]) and (not IsAlNum(string[1]))) or
                            ((not IsAlNum(string[0])) and IsAlNum(string[1]))) and \
                        (string[0] != ' ' and string[1] != ' '):
                    # this was originally prevent junk chars to be added
                    # since now the vita text is pretty better with almost no junk chars
                    # I just log it out and comment this
                    # continue
                    print('alpha string:{}'.format(string))
                dst_list.append((fname, string))

        src.close()
    return dst_list


def FormatString(index, jp_line, cn_line):
    res = "○%08d○%s\n●%08d●%s\n\n"%(index, jp_line, index, cn_line)
    return res

def OutputMapFile(text_list, jpcn_dic, dst_file, report):

    idx = 0
    for fname, key_str in text_list:
        if not key_str in jpcn_dic:
            report.write('{}:{}\n'.format(fname, key_str))
            continue

        value_str = jpcn_dic[key_str]
        dst_file.write(FormatString(idx, key_str, value_str))
        idx += 1


def main():

    report_not_match = open('ReportNotMatch.txt', 'w', encoding='utf16')
    report_group_error = open('ReportGroupError.txt', 'w', encoding='utf16')

    vita_flist = Common.Walk('vita_jp_txt')
    text_list = ReadVitaGroups(vita_flist)

    jp_flist = Common.Walk('psp_jp_txt')
    cn_flist = Common.Walk('pc_cn_txt')
    jpcn_dic = MakeStrDic(jp_flist, cn_flist, report_group_error)

    dst = open('JpCnMap.txt', 'w', encoding='utf16')

    OutputMapFile(text_list, jpcn_dic, dst, report_not_match)

    dst.close()
    report_not_match.close()

    print('Process done.')


if __name__ == '__main__':
    main()