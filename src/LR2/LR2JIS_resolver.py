import sys, os
import codecs

chars = []
lr2glyphs = []
params = ['SHIFT-JIS.txt', 'font.lr2font']
if (len(sys.argv) > 2):
    params[0] = sys.argv[0]
    params[1] = sys.argv[1]

def getJISord(c):
    '''
    i = 0
    try:
        for x in c.encode('shift-jis'):
            i *= 256
            i += x
        return i
    except:
        return ord(c)
    '''
    return ord(c)

# cache some necessary char first
chars.append(' ')
chars.append('\n')

# load original file SHIFT-JIS.txt
# and we add tab / break char for special.
with codecs.open(params[0], 'r', 'utf-8') as f:
    txt = f.read()
    for char in txt:
        if (char not in chars):
            if (char != '\r'):
                chars.append(char)

#chars.sort()

# load compiled file font.lr2font
with codecs.open(params[1], 'r', 'shift-jis') as f:
    for line in f:
        if (line[:2] != '#R'):
            continue
        params = line.split(',')
        # order: texid, y, x, lr2glyphid
        lr2glyphs.append( (int(params[2]), int(params[4]), int(params[3]), int(params[1])) )

maxglyphid = lr2glyphs[-1][3]

#lr2glyphs.sort()

# check length
if (len(chars) != len(lr2glyphs)):
    print('len not fit: chrs %d, lr2glyphs %d' % (len(chars), len(lr2glyphs)))
    exit(-1)

# prepare map
glyphmap = [0 for x in range(maxglyphid + 1)]
for (char, g) in zip(chars, lr2glyphs):
    glyphmap[g[3]] = ord(char)

# debug result (1)
i = 0
with codecs.open(params[0]+'.out', 'w', 'utf-8') as f:
    for char in chars:
        f.write(char)
        i += 1
        if (i % 64 == 0):
            f.write('\n')

# debug result (2)
i = 0
with codecs.open(params[0]+'.mapout', 'w', 'utf-8') as f:
    for gidx in glyphmap:
        f.write('LR2glyph %d (%s) = %d\n' % (i, chr(glyphmap[i]), gidx))
        i+=1

# write result
with open('LR2JIS.h', 'w') as f:
    f.write('uint16_t _LR2GlyphID[%d] = {' % (maxglyphid + 1))
    for unichrpt in glyphmap:
        f.write('%d,\n' % unichrpt)
    f.write('};\n\n')
    f.write('''uint16_t ConvertLR2JIStoUTF16(uint16_t lr2_code)
{
  if (lr2_code > %d) return 0;
  return _LR2GlyphID[lr2_code];
}
''' % (maxglyphid + 1))