import os

def sanitize_var_name(name):
    return name.replace('.', '_').replace('-', '_')

def mp4_to_c_array(mp4_path, prefix):
    with open(mp4_path, 'rb') as f:
        data = f.read()
    array_content = ', '.join(f'0x{b:02x}' for b in data)
    base_name = os.path.splitext(os.path.basename(mp4_path))[0]
    var_name = sanitize_var_name(f'data_{prefix}_{base_name}')
    return f'unsigned char {var_name}[] = {{ {array_content} }};\nunsigned int {var_name}_len = {len(data)};\n'

def main():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    output_path = os.path.join(base_dir, 'bytes.txt')

    with open(output_path, 'w') as out_file:
        for root, _, files in os.walk(base_dir):
            for file in files:
                if file.endswith('.mp4'):
                    mp4_path = os.path.join(root, file)
                    # 相对路径中提取目录名作为前缀
                    relative_path = os.path.relpath(mp4_path, base_dir)
                    parts = relative_path.split(os.sep)
                    if len(parts) >= 2:
                        prefix = sanitize_var_name(parts[0])  # e.g., "20" or "64"
                    else:
                        prefix = 'root'
                    try:
                        print(f'Processing: {relative_path}')
                        c_array = mp4_to_c_array(mp4_path, prefix)
                        out_file.write(c_array + '\n')
                    except Exception as e:
                        print(f"Failed to process {relative_path}: {e}")

    print(f'\n✅ Done. Output written to: {output_path}')

if __name__ == '__main__':
    main()
