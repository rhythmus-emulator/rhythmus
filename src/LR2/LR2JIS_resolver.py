import sys, os
import codecs

chars = []
lr2glyphs = []
params = ['SHIFT-JIS.txt', 'font.lr2font']
if (len(sys.argv) > 2):
    params[0] = sys.argv[0]
    params[1] = sys.argv[1]

# load original file SHIFT-JIS.txt
with codecs.open(params[0], 'r', 'utf-8') as f:
    for line in f:
        for char in line:
            if (char not in chars):
                if (char != '\r'):  # left \t, \n and space
                    chars.append(char)

chars.sort()

# load compiled file font.lr2font
with codecs.open(params[1], 'r', 'shift-jis') as f:
    for line in f:
        if (line[:2] != '#R'):
            continue
        params = line.split(',')
        # order: texid, y, x, lr2glyphid
        lr2glyphs.append( (int(params[2]), int(params[4]), int(params[3]), int(params[1])) )

maxglyphid = lr2glyphs[-1][3]

lr2glyphs.sort()

# check length
if (len(chars) != len(lr2glyphs)):
    print('len not fit: chrs %d, lr2glyphs %d' % (len(chars), len(lr2glyphs)))
    exit(-1)

# prepare map
glyphmap = [0 for x in range(maxglyphid + 1)]
for (char, g) in zip(chars, lr2glyphs):
    glyphmap[g[3]] = ord(char)

# write results
i = 0
with codecs.open(params[0]+'.out', 'w', 'utf-8') as f:
    for char in chars:
        f.write(char)
        i += 1
        if (i % 64 == 0):
            f.write('\n')


with open('LR2JIS.h', 'w') as f:
    f.write('uint16_t _LR2GlyphID[%d] = {' % (maxglyphid + 1))
    for g in glyphmap:
        f.write('%d,\n' % g)
    f.write('};\n\n')
    f.write('''uint16_t ConvertLR2JIStoUTF16(uint16_t lr2_code)
{
  if (lr2_code > %d) return 0;
  return _LR2GlyphID[lr2_code];
}
''' % maxglyphid)