#!/usr/bin/env python3
import os
import sys
import shutil
import argparse

def main():
    parser = argparse.ArgumentParser(description='Create a new Linux driver project from Driver_Template')
    parser.add_argument('name', help='Name of the new driver (e.g., dht11_drv)')
    parser.add_argument('--path', '-p', 
                        help='Target directory path (default: same directory as template)')
    
    args = parser.parse_args()
    
    template_path = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    
    if args.path:
        new_project_path = os.path.abspath(os.path.join(args.path, args.name))
    else:
        repo_root = os.path.dirname(template_path)
        new_project_path = os.path.join(repo_root, args.name)
    
    if os.path.exists(new_project_path):
        print(f'Error: Project "{args.name}" already exists at {new_project_path}')
        sys.exit(1)
    
    parent_dir = os.path.dirname(new_project_path)
    if not os.path.exists(parent_dir):
        os.makedirs(parent_dir, exist_ok=True)
    
    print(f'Creating new driver project: {args.name}')
    print(f'Template: {template_path}')
    print(f'Target: {new_project_path}')
    
    ignore_patterns = [
        '.git',
        '__pycache__',
        '*.ko',
        '*.o',
        '*.mod.c',
        '*.mod',
        '*.symvers',
        '*.order',
        '.tmp_versions',
        '.*.cmd',
        'app/led_app',
    ]
    
    def ignore_func(dir, files):
        ignored = []
        for f in files:
            for pattern in ignore_patterns:
                if f == pattern or (pattern.startswith('*') and f.endswith(pattern[1:])):
                    ignored.append(f)
                    break
        return ignored
    
    shutil.copytree(template_path, new_project_path, ignore=ignore_func)
    
    driver_makefile = os.path.join(new_project_path, 'driver', 'Makefile')
    with open(driver_makefile, 'r') as f:
        content = f.read()
    
    content = content.replace('obj-m += led_drv.o', f'obj-m += {args.name}.o')
    
    with open(driver_makefile, 'w') as f:
        f.write(content)
    
    print(f'\n✅ Project created successfully!')
    print(f'\nNext steps:')
    print(f'  1. cd {new_project_path}')
    print(f'  2. Replace driver/led_drv.c with your driver code (rename to {args.name}.c)')
    print(f'  3. Replace app/led_app.c with your test application (optional)')
    print(f'  4. Run: ./scripts/generate_compile_commands.py')
    print(f'  5. Run: make')

if __name__ == '__main__':
    main()
