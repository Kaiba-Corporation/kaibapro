# Adds a limit of 5 tries to "goto cancel" loops in card scripts.
# Usage: python limit_cancel.py SOURCE_SCRIPT_DIR OUT_DIR [PREFER_DIR]
# PREFER_DIR holds already-fixed scripts to start from instead of the source copy.
import os, re, sys

src, out = sys.argv[1], sys.argv[2]
prefer = sys.argv[3] if len(sys.argv) > 3 else None
label_rx = re.compile(rb'^([ \t]*)::(\w*cancel)::[ \t]*(\r?)$', re.M)
LIMIT = 5
done = 0
for name in sorted(os.listdir(src)):
    if not re.fullmatch(r'c\d+\.lua', name):
        continue
    path = os.path.join(prefer, name) if prefer and os.path.exists(os.path.join(prefer, name)) else os.path.join(src, name)
    data = open(path, 'rb').read()
    if not label_rx.search(data):
        continue
    if b'bt_tries_' in data:            # already limited
        if os.path.abspath(path) != os.path.abspath(os.path.join(out, name)):
            os.makedirs(out, exist_ok=True)
            open(os.path.join(out, name), 'wb').write(data)
        done += 1
        continue
    def repl(m):
        ind, label, cr = m.group(1), m.group(2).decode(), m.group(3)
        var = ('bt_tries_' + label).encode()
        nl = cr + b'\n'
        return (ind + b'local ' + var + b'=0 --Bot Theater: at most %d tries' % LIMIT + nl +
                ind + b'::' + label.encode() + b'::' + nl +
                ind + var + b'=' + var + b'+1 if ' + var + b'>%d then return end' % LIMIT + cr)
    new = label_rx.sub(repl, data)
    os.makedirs(out, exist_ok=True)
    open(os.path.join(out, name), 'wb').write(new)
    done += 1
print(done, 'scripts written to', out)
