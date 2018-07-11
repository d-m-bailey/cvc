# -*- mode: python -*-
 
block_cipher = None
 
 
home = "./"
src = home

a = Analysis([src + "check_cvc.py"],
             pathex=[src],
             binaries=None,
             datas=[(src + "/summary.kv", ".")],
             hiddenimports=["importlib", "six", "packaging", 
                            "packaging.version", "packaging.specifiers", "packaging.requirements"],
             hookspath=[],
             runtime_hooks=[],
             excludes=[],
             win_no_prefer_redirects=False,
             win_private_assemblies=False,
             cipher=block_cipher)
pyz = PYZ(a.pure, a.zipped_data,
             cipher=block_cipher)
exe = EXE(pyz,
          a.scripts,
          a.binaries,
          a.zipfiles,
          a.datas,
          name='check_cvc',
          debug=False,
          strip=False,
          upx=True,
          console=True )
