Import("env")
try:
    import jsmin 
    import htmlmin
    import csscompressor
except ImportError:
    env.Execute("$PYTHONEXE -m pip install htmlmin jsmin csscompressor")
env.Execute("$PYTHONEXE $PROJECT_DIR/scripts/generate_headers.py -p $PROJECT_DIR -n")
