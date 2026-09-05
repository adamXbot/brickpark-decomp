# Reproducible documentation checks

Run the following from the repository root with Python 3. It reads tracked source and documentation, invokes read-only Git commands, and does not compile or modify files. It verifies structural completeness and source-linked callback assignments; human review of behavior remains recorded in the audit pages.

~~~python
from pathlib import Path
from collections import Counter
import hashlib, re, subprocess

root = Path.cwd()
assert (root / 'docs/SCOPE_J_runtime_spec.md').exists(), 'Run from repository root'
pages = [root / 'docs/RUNTIME_SPEC.md', *sorted((root / 'docs/runtime').glob('*.md'))]
errors, link_count = [], 0

def require(condition, message):
    if not condition:
        errors.append(message)

def headings(path):
    seen, result = Counter(), set()
    for line in re.findall(r'^#{1,6} (.+)$', path.read_text(), re.M):
        slug = re.sub(r'[^\w\- ]', '', re.sub(r'<[^>]*>', '', line).lower()).replace(' ', '-')
        result.add(slug + (f'-{seen[slug]}' if seen[slug] else ''))
        seen[slug] += 1
    return result

for page in pages:
    body = page.read_text()
    # Do not mistake the checker examples in fenced code for document links.
    prose = re.sub(r'^(```|~~~)[^\n]*\n.*?^\1\s*$', '', body, flags=re.M | re.S)
    for label, url in re.findall(r'(?<!!)\[([^\]\n]+)\]\(([^)\n]+)\)', prose):
        if '://' in url:
            continue
        link_count += 1
        name, _, fragment = url.partition('#')
        target = (page.parent / name).resolve() if name else page
        require(target.exists(), f'{page.name}: missing {url}')
        if target.exists() and fragment and target.suffix == '.md':
            require(fragment in headings(target), f'{page.name}: missing anchor {url}')
    expected = None
    for line_number, line in enumerate(prose.splitlines(), 1):
        if not line.startswith('|'):
            expected = None
            continue
        count = len(re.findall(r'(?<!\\)\|', line))
        if expected is None:
            expected = count
        require(count == expected, f'{page.name}:{line_number}: malformed table')

# Where an audit records a source fingerprint, require it to match this tree.
fingerprint_count = 0
for page in pages:
    if not page.name.endswith('-audit.md'):
        continue
    for line in page.read_text().splitlines():
        match = re.match(r'\| \[[^]]+\]\(../../(LEGOLAND/[^)]+\.c)\).*\| `([0-9a-f]{12})` \|$', line)
        if match:
            name, prefix = match.groups()
            fingerprint_count += 1
            require(hashlib.sha256((root / name).read_bytes()).hexdigest().startswith(prefix), f'Stale source audit: {name}')

tracked = set(subprocess.check_output(['git', 'ls-files', 'LEGOLAND/*.c'], text=True).splitlines())
coverage = (root / 'docs/runtime/coverage.md').read_text()
rows = re.findall(r'^\| \[([^]]+\.c)\]\(../../(LEGOLAND/[^)]+)\) \| (documented|partial|not yet) \|', coverage, re.M)
require(len(rows) == len(tracked) == len({row[1] for row in rows}), 'Duplicate/missing coverage row')
require({row[1] for row in rows} == tracked, 'Coverage differs from tracked C inventory')
primary = None
for line in coverage.splitlines():
    match = re.match(r'Primary page: \[[^]]+\]\(([^)]+)\)', line)
    if match:
        primary = root / 'docs/runtime' / match[1]
    match = re.match(r'\| \[[^]]+\.c\]\(../../(LEGOLAND/[^)]+)\)', line)
    if match:
        require(primary is not None and '../../' + match[1] in primary.read_text(), f'No primary-page citation: {match[1]}')

# Validate callback labels against actual function definitions, not extern aliases.
implementations = set()
for name in tracked:
    source = (root / name).read_text()
    for match in re.finditer(r'//\s*(?:WIP-)?FUNCTION:\s*LEGOLAND\s+(0x[0-9a-fA-F]+)([^{}]*?)\{', source):
        signature = re.sub(r'/\*.*?\*/|//[^\n]*', '', match[2], flags=re.S)
        names = re.findall(r'\b([A-Za-z_]\w*)\s*\(', signature)
        if names:
            implementations.add((name, names[-1], int(match[1], 16)))
callbacks = (root / 'docs/runtime/callbacks.md').read_text()
linked = re.findall(r'\[([A-Za-z_]\w*)\]\(../../(LEGOLAND/[^)]+)\) \(`(0x[0-9a-fA-F]+)`\)', callbacks)
for name, path, address in linked:
    require((path, name, int(address, 16)) in implementations, f'Wrong callback: {path}:{name}:{address}')

# Each explicit final direct-registration assignment must appear in its class row.
fields = dict(add='98', remove='9c', draw='a0', create='a4', activate='a8', destroy='ac', interact='b0', load='b8', save='bc')
assignment_count = 0
for file in ['screen.c', 'interfaces.c', 'ridesave.c', 'castleobj.c']:
    source = (root / 'LEGOLAND' / file).read_text()
    constants = dict(re.findall(r'(k\w+)\[\]\s*=\s*"([^"]+)"', source))
    declarations = {}
    for line in source.splitlines():
        match = re.search(r'extern\s+.*?\b(\w+)\s*\([^;]*;.*?(0x[0-9a-fA-F]{6,8})', line)
        if match:
            declarations[match[1]] = int(match[2], 16)
    section = callbacks.split('## Registrations in ' + file + '\n', 1)[1].split('\n## ', 1)[0]
    for match in re.finditer(r'if\s*\(([^{}]*?)\)\s*\{([^{}]*?)\}', source):
        condition, body = match.groups()
        writes = re.findall(r'def->cb_(\w+)\s*=\s*(\w+)\s*;', body)
        if not writes or not re.search('NameCompare|strcmp', condition):
            continue
        classes = re.findall(r'"([^"]+)"', condition) or [constants[x] for x in re.findall(r'\bk\w+\b', condition) if x in constants]
        require(bool(classes), f'Unparsed class in {file}')
        final = {fields.get(slot, slot): symbol for slot, symbol in writes}
        assignment_count += len(final) * len(classes)
        for cls in classes:
            doc_rows = [line for line in section.splitlines() if line.startswith('|') and '`' + cls + '`' in line.split('|')[1]]
            require(len(doc_rows) == 1, f'Missing/duplicate class: {file}:{cls}')
            if len(doc_rows) != 1:
                continue
            for slot, symbol in final.items():
                address = declarations.get(symbol)
                if address is None:
                    matches = {va for path, function, va in implementations if function == symbol}
                    address = next(iter(matches)) if len(matches) == 1 else None
                require(address is not None, f'Unresolved assignment address: {symbol}')
                pattern = r'`\+' + slot + r'`[^;|]*`0x' + (f'{address:08x}' if address is not None else 'UNKNOWN') + '`'
                require(re.search(pattern, doc_rows[0]) is not None, f'Wrong slot: {cls}+{slot}:{symbol}')

required = {'boat layout': '0x3f4', 'track joint': 'TrackJoint', 'solver': 'RK4', 'z commands': 'ylast', 'CSP': '.csp', 'tiredness': '1000,2400,4000,7000', 'flume null-parent': 'LFCorner_Place', 'bisection': 'bisection', 'report line index': '1-based', 'caption pitch': 'pitch', 'Joust freeze': 'freeze', 'Balloonz': 'Balloonz'}
all_text = '\n'.join(page.read_text() for page in pages)
for topic, token in required.items():
    require(token.lower() in all_text.lower(), f'Missing named topic: {topic}')
base = subprocess.check_output(['git', 'merge-base', 'HEAD', 'origin/main'], text=True).strip()
changed = subprocess.check_output(['git', 'diff', '--name-only', base], text=True).splitlines()
changed += [line[3:] for line in subprocess.check_output(['git', 'status', '--porcelain', '--untracked-files=all'], text=True).splitlines() if line.startswith('?? ')]
for path in changed:
    require(path == 'docs/RUNTIME_SPEC.md' or (path.startswith('docs/runtime/') and path.endswith('.md')), f'Outside scope: {path}')
subprocess.run(['git', 'diff', '--check', base], check=True)
print(f'{len(pages)} documents; {link_count} local links; {len(rows)} source rows; {len(linked)} callback links; {assignment_count} direct assignments')
print('Coverage:', dict(Counter(row[2] for row in rows)), '; source fingerprints:', fingerprint_count)
if errors:
    raise SystemExit('\n'.join(errors))
print('PASS: links, anchors, tables, source inventory, callback names/slots, required topics and allowed paths')
~~~
