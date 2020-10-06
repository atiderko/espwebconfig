#!/usr/bin/env python3

from jsmin import jsmin as jsminify
from htmlmin import minify as htmlminify
from csscompressor import compress as cssminify
import gzip
import sys
import os.path
import argparse
import re
from glob import glob

TARGET_TEMPLATE = '''const char {constant}[] PROGMEM = R"=====(
{minidata}
)=====";
'''
TARGET_GZIP_TEMPLATE = '''const uint8_t {constant}_GZIP[{gziplen}] PROGMEM = {{ {gzipdata} }};'''

def parse_arguments(args=None):
    parser = argparse.ArgumentParser(
        description='Generates header files by minifying and gzipping HTML, JS and CSS source files.')
    parser.add_argument('--project_dir', '-p', dest='project_dir', default='',
                        help='Target directory containing C header files OR one C header file')
    parser.add_argument('--storemini', '-m', action='store_true', dest='storemini',
                        help='Store intermediate minified files next to the originals (i.e. only write to the C header files)')
    parser.add_argument('--notminify', '-n', action='store_true', dest='notminify',
                        help='Do not minify, e.g. for debug')
    args = parser.parse_args(args)
    return args


def get_context(infile, outfile):
    infile	= os.path.realpath(infile)
    prefix, name, ext 	= (os.path.basename(os.path.dirname(infile)), os.path.basename(infile).split(os.path.extsep)[0], os.path.basename(infile).split(os.path.extsep)[-1] )
    if name == ext:
        ext = 'html'
    ext = ext.strip('.').lower()
    if prefix.lower() == ext:
        # use subdirectory as midname
        prefix = os.path.basename(os.path.dirname(os.path.dirname(infile)))
    if ext == 'htm':
        ext = 'html'
    name = os.path.basename(name)
    indir	= os.path.dirname(infile)
    if os.path.isdir(outfile):
        outdir 	= os.path.realpath(outfile)
        # outdir = os.path.join(outdir, 'generated')
        os.makedirs(outdir, exist_ok=True)
        outfilename	= '%s%s%s.h' % (prefix, name.capitalize(), ext.upper())
        outfile	= os.path.realpath(os.path.sep.join([outdir, outfilename]))
    else:
        print('OUTFILE', outfile)
        outfile	= os.path.realpath(outfile)
        outdir	= os.path.dirname(outfile)
        outfilename	= os.path.basename(outfile)
    minifile	= re.sub('\.([^.]+)$', '.min.\\1', infile) if not '.min.' in infile else infile
    constant	= '%s_%s_%s' % (ext.upper(), prefix.upper(), name.upper())
    return locals()
    
def perform_gzip(c):
    compressed = gzip.compress(bytes(c['minidata'], 'utf-8'))
    c['gzipdata'] = ','.join([ str(b) for b in compressed ])
    c['gziplen'] = len(compressed)
    return c
    
def perform_minify(c, notminify=False):
    with open(c['infile']) as infile:
        minifier = {
            'css': cssminify,
            'js': jsminify,
            'json': jsminify,
            'html': htmlminify
        }.get(c['ext']) or htmlminify
        if notminify:
            c['minidata'] = infile.read()
        else:
            print('  Minify %s' % (c['infile']))
            c['minidata'] = minifier(infile.read())
        perform_gzip(c)
    return c

def process_file(infile, outdir, storemini=False, notminify=False):
    print('Processing file %s' % infile)
    c = get_context(infile, outdir)
    c = perform_minify(c, notminify=notminify)
    if storemini:
        if c['infile'] == c['minifile']:
            print('  Original file is already minified, refusing to overwrite it')
        else:
            print('  Writing minified file %s' % c['minifile'])
            with open(c['minifile'], 'w+') as minifile:
                minifile.write(c['minidata'])
    template = TARGET_TEMPLATE.format(**c)
    changed = True
    if os.path.exists(c['outfile']):
        # check for changes
        with open(c['outfile'], 'r') as outfile:
            # since gzip returns not reproducible output (until v3.8) we need to extract no compressed part
            current = outfile.read()
            idx = current.find(')=====";\n')
            if idx > 0:
                found_part = current[0:idx+9]
                changed = not (template == found_part)
    if changed:
        with open(c['outfile'], 'w+') as outfile:
            print('  Using C constant names %s and %s_GZIP' % (c['constant'], c['constant']))
            print('  Writing C header file %s' % c['outfile'])
            print('  Data length:      %s' % len(c['minidata']))
            print('  GZIP data length: %s' % c['gziplen'])
            print('  rate:             %.2f' % (c['gziplen'] / len(c['minidata'])))
            outfile.write(template)
            outfile.write(TARGET_GZIP_TEMPLATE.format(**c))
    else:
        print('  no changes, skip')
    return c['outfile']
  
        
def process_dir(sourcedir, outdir, recursive=True, storemini=True, notminify=False):
    pattern = r'/*\.(css|js|json|htm|html)$'
    files = glob(sourcedir + '/**/*', recursive=True)+glob(sourcedir + '/*') if recursive else glob(sourcedir + '/*')
    files_filtered = set(filter(re.compile(pattern).search, files) )
    # search for HTML files without extension (for simple local testing of web structure)
    for f in files:
        try:
            if f not in files_filtered and os.path.isfile(f):
                with open(f) as infile:
                    if infile.readline().startswith('<!DOCTYPE html>'):
                        files_filtered.add(f)
        except Exception:
            pass
    result = []  # list with all generated files
    for f in files_filtered:
        if not '.min.' in f:
            gf = process_file(f, outdir, storemini, notminify)
            result.append(gf)
        # elif not os.path.isfile(f.replace('.min.', '.')):
        #     process_file(f, outdir, storemini)
    return result

def main(project_dir, storemini=False, notminify=False):
    pdir = project_dir
    web = os.path.realpath(os.path.join(pdir, 'web'))
    src  = os.path.realpath(os.path.join(pdir, 'src', 'generated'))
    os.makedirs(src, exist_ok=True)
    files = glob(src + '/**/*', recursive=True)
    gf = process_dir(web, src, recursive = True, storemini=storemini, notminify=notminify)
    # remove old files
    for rmf in list(set(files) - set(gf)):
        print('Delete header of removed web file: %s' % rmf)
        os.remove(rmf)


if __name__ == '__main__' and 'get_ipython' not in dir():
    args = parse_arguments()
    print('minify: %s' % ('false' if args.notminify else 'true'))
    print('storemini: %s' % ('true' if args.storemini else 'false'))
    print('project_dir: %s' % (args.project_dir))
    library_dir = os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)), '..'))
    if args.project_dir != library_dir:
	    # generate library headers
        main(library_dir, args.storemini, args.notminify)
    main(args.project_dir, args.storemini, args.notminify)

