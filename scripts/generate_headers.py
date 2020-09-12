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
        description="Generates header files by minifying and gzipping HTML, JS and CSS source files.")
    parser.add_argument("--project_dir", "-p", dest="project_dir", default="",
                        help="Target directory containing C header files OR one C header file")
    parser.add_argument("--storemini", "-m", action="store_true", dest="storemini",
                        help="Store intermediate minified files next to the originals (i.e. only write to the C header files)")
    parser.add_argument("--gzip", "-z", action="store_true", dest="gzip",
                        help="Generate gzip instead of plain text template in header files.")
    args = parser.parse_args(args)
    return args


def get_context(infile, outfile):
    infile	= os.path.realpath(infile)
    prefix, name, ext 	= (os.path.basename(os.path.dirname(infile)), os.path.basename(infile).split(os.path.extsep)[0], os.path.basename(infile).split(os.path.extsep)[-1] )
    if name == ext:
        ext = 'html'
    ext = ext.strip(".").lower()
    if prefix.lower() == ext:
        # use subdirectory as midname
        prefix = os.path.basename(os.path.dirname(os.path.dirname(infile)))
    if ext == "htm":
        ext = 'html'
    name = os.path.basename(name)
    indir	= os.path.dirname(infile)
    if os.path.isdir(outfile):
        print("DIR:", outfile)
        outdir 	= os.path.realpath(outfile)
        # outdir = os.path.join(outdir, 'generated')
        os.makedirs(outdir, exist_ok=True)
        outfilename	= "%s%s%s.h" % (prefix, name.capitalize(), ext.upper())
        outfile	= os.path.realpath(os.path.sep.join([outdir, outfilename]))
    else:
        outfile	= os.path.realpath(outfile)
        outdir	= os.path.dirname(outfile)
        outfilename	= os.path.basename(outfile)
    minifile	= re.sub('\.([^.]+)$', '.min.\\1', infile) if not ".min." in infile else infile
    constant	= '%s_%s_%s' % (ext.upper(), prefix.upper(), name.upper())
    return locals()
    
def perform_gzip(c):
    compressed = gzip.compress(bytes(c['minidata'], 'utf-8'))
    c['gzipdata'] = ','.join([ str(b) for b in compressed ])
    c['gziplen'] = len(compressed)
    print("  GZIP data length: %s" % c['gziplen'])
    return c
    
def perform_minify(c, usegzip=False):
    with open(c['infile']) as infile:
        minifier = {
            'css': cssminify, 
            'js': jsminify, 
            'html': htmlminify
        }.get(c['ext']) or htmlminify
        print("  Using %s minifier" % c['ext'])
        c['minidata'] = minifier(infile.read())
    if usegzip:
        return perform_gzip(c)
    return c


def process_file(infile, outdir, storemini=False, usegzip=False):
    print("Processing file %s" % infile)
    c = get_context(infile, outdir)
    c = perform_minify(c, usegzip)
    if storemini:
        if c['infile'] == c['minifile']:
            print("  Original file is already minified, refusing to overwrite it")
        else:
            print("  Writing minified file %s" % c['minifile'])
            with open(c['minifile'], 'w+') as minifile:
                minifile.write(c['minidata'])
    with open(c['outfile'], 'w+') as outfile:
        print("  Using C constant names %s and %s_GZIP" % (c['constant'], c['constant']))
        print("  Writing C header file %s" % c['outfile'])
        if usegzip:
            outfile.write(TARGET_GZIP_TEMPLATE.format(**c))
        else:
            outfile.write(TARGET_TEMPLATE.format(**c))
   
        
def process_dir(sourcedir, outdir, recursive=True, storemini=True, usegzip=False):
    pattern = r'/*\.(css|js|htm|html)$'
    files = glob(sourcedir + "/**/*", recursive=True)+glob(sourcedir + "/*") if recursive else glob(sourcedir + "/*")
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
    for f in files_filtered:
        if not '.min.' in f:
            process_file(f, outdir, storemini, usegzip)
        # elif not os.path.isfile(f.replace(".min.", ".")):
        #     process_file(f, outdir, storemini)

def main(args):
    pdir = args.project_dir
    if not pdir:
        pdir = os.path.join(os.path.dirname(os.path.realpath(__file__)), "..")
    web = os.path.realpath(os.path.join(pdir, "web"))
    src  = os.path.realpath(os.path.join(pdir, "src", "generated"))
    os.makedirs(src, exist_ok=True)
    for root, dirs, files in os.walk(src, topdown=False):
        for name in files:
            os.remove(os.path.join(root, name))
        for name in dirs:
            os.rmdir(os.path.join(root, name))
    print("args.storemini:", args.storemini)
    print("args.gzip:", args.gzip)
    process_dir(web, src, recursive = True, storemini=args.storemini, usegzip=args.gzip)

if __name__ == "__main__" and "get_ipython" not in dir():
    main(parse_arguments())
