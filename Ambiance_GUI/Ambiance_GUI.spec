# -*- mode: python ; coding: utf-8 -*-

from PyInstaller.utils.hooks import collect_submodules, collect_data_files

a = Analysis(
    ['Ambiance_GUI.py'],
    pathex=['.'],
    binaries=[],
    datas=collect_data_files('serial') + collect_data_files('bleak'),
    hiddenimports=collect_submodules('serial') 
                  + collect_submodules('bleak.backends')
                  + ['serial.tools.list_ports', 'bleak.backends.corebluetooth'],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
    optimize=0,
)

pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    [],
    exclude_binaries=True,
    name='Ambiance_GUI',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    console=False,
)

coll = COLLECT(
    exe,
    a.binaries,
    a.datas,
    strip=False,
    upx=True,
    upx_exclude=[],
    name='Ambiance_GUI',
)

app = BUNDLE(
    coll,
    name='Ambiance.app',
    icon='Ambiance.icns'
)