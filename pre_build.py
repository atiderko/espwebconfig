Import("env")
env.Execute("python $PROJECT_DIR/scripts/generate_headers.py -p $PROJECT_DIR")
