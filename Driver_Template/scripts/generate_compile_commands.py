#!/usr/bin/env python3
import os
import json
import argparse

def main():
    parser = argparse.ArgumentParser(description='Generate compile_commands.json for Linux driver project')
    parser.add_argument('--kdir', default='/home/gm/Workspace/linux_sdk/luckfox_rk3506_sdk/kernel',
                        help='Kernel source directory')
    parser.add_argument('--cross-compile', 
                        default='/home/gm/Workspace/linux_sdk/luckfox_rk3506_sdk/prebuilts/gcc/linux-x86/arm/gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-',
                        help='Cross compiler prefix')
    parser.add_argument('--output', default='compile_commands.json',
                        help='Output file path')
    
    args = parser.parse_args()
    
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    driver_dir = os.path.join(project_root, 'driver')
    app_dir = os.path.join(project_root, 'app')
    
    compile_commands = []
    
    kernel_includes = [
        f'-I{args.kdir}/arch/arm/include',
        f'-I{args.kdir}/arch/arm/include/generated',
        f'-I{args.kdir}/include',
        f'-I{args.kdir}/include/uapi',
        f'-I{args.kdir}/include/generated',
        f'-I{args.kdir}/include/generated/uapi',
        f'-I{args.kdir}/arch/arm/include/uapi',
    ]
    
    common_flags = [
        '-nostdinc',
        '-D__KERNEL__',
        '-DMODULE',
        '-Wall',
        '-Wundef',
        '-Wstrict-prototypes',
        '-Wno-trigraphs',
        '-fno-strict-aliasing',
        '-fno-common',
        '-fshort-wchar',
        '-std=gnu11',
        '-O2'
    ]
    
    gcc = f'{args.cross_compile}gcc'
    
    driver_files = [f for f in os.listdir(driver_dir) if f.endswith('.c')]
    for file in driver_files:
        cmd = [gcc, '-c'] + kernel_includes + common_flags + [file]
        compile_commands.append({
            'directory': driver_dir,
            'command': ' '.join(cmd),
            'file': file
        })
    
    app_files = [f for f in os.listdir(app_dir) if f.endswith('.c')]
    for file in app_files:
        cmd = [gcc, '-c', f'-I{args.kdir}/include', '-std=gnu11', '-O2', file]
        compile_commands.append({
            'directory': app_dir,
            'command': ' '.join(cmd),
            'file': file
        })
    
    output_path = os.path.join(project_root, args.output)
    with open(output_path, 'w') as f:
        json.dump(compile_commands, f, indent=2)
    
    print(f'Generated {output_path} with {len(compile_commands)} entries')

if __name__ == '__main__':
    main()
