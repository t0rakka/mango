project(
    'example', 'cpp', 'c',
    meson_version: '>= 0.48.0',
    default_options : [
        'c_std=c11', 'cpp_std=c++17'
    ]
)

mango_dep = dependency('mango')
mango_opengl_dep = dependency('mango-opengl')
GL_dep = dependency('GL')
png_dep = dependency('libpng')
jpeg_dep = dependency('libjpeg')

# --------------------------------------------------------------------------------
# core tests
# --------------------------------------------------------------------------------

test_progs = [
    'aes',
    'checksum',
    'hash',
    'compress',
    'threads',
    'pathtest',
    'particle',
    'mathtest',
]

foreach p : test_progs
    executable('test_' + p, 'test/' + p + '.cpp', dependencies : mango_dep)
endforeach

# --------------------------------------------------------------------------------
# OpenGL tests
# --------------------------------------------------------------------------------

# TODO: make these optional -or- depend on the GL_deps

opengl_progs = [
    'basic',
    'imageview',
    'animation',
    'mandelbrot',
]

foreach p : opengl_progs
    executable('opengl_' + p, 'opengl/' + p + '.cpp', dependencies : [mango_dep, mango_opengl_dep, GL_dep])
endforeach

# --------------------------------------------------------------------------------
# utils
# --------------------------------------------------------------------------------

utils_progs = [
    'snitch',
    'unsnitch',
]

foreach p : utils_progs
    executable(p, 'utils/' + p + '.cpp', dependencies : mango_dep)
endforeach

# --------------------------------------------------------------------------------
# image tests
# --------------------------------------------------------------------------------

executable('image_webp',          
    'image/webp/webp.cpp', 
    dependencies : mango_dep)
executable('image_jpeg_reader',   
    'image/jpeg_reader/jpeg_reader.cpp', 
    dependencies : mango_dep)
executable('image_jpeg_debugger', 
    'image/jpeg_debugger/jpeg_debugger.cpp', 
    dependencies : mango_dep)
executable('image_icc_p3',        
    'image/icc/p3.cpp', 
    dependencies : mango_dep)
executable('image_blitter',       
    'image/blitter/blitter.cpp', 
    dependencies : mango_dep)
executable('image_png_benchmark',
    'image/png_benchmark/png_benchmark.cpp', 
    'image/png_benchmark/lodepng/lodepng.cpp',
    'image/png_benchmark/spng/spng.c',
    'image/png_benchmark/fpng/fpng.cpp',
    'image/png_benchmark/fpng/Crc32.cpp',
    dependencies : [mango_dep, png_dep])
executable('image_jpeg_benchmark',
    'image/jpeg_benchmark/jpeg_benchmark.cpp', 
    'image/jpeg_benchmark/jpeg-compressor/jpgd.cpp', 
    'image/jpeg_benchmark/jpeg-compressor/jpge.cpp', 
    'image/jpeg_benchmark/jpegdec/JPEGDEC.cpp', 
    dependencies : [mango_dep, jpeg_dep])

# --------------------------------------------------------------------------------
# NOTE: If there is a way to avoid this hack ...
# --------------------------------------------------------------------------------

copy = find_program('meson.copy.py')

run_command(copy, 'opengl/data/ball.png', 'data/ball.png')
run_command(copy, 'opengl/data/clock.png', 'data/clock.png')
run_command(copy, 'opengl/data/dude.gif', 'data/dude.gif')
run_command(copy, 'opengl/data/o_sample.png', 'data/o_sample.png')

run_command(copy, 'image/blitter/conquer.jpg', 'conquer.jpg')
run_command(copy, 'image/icc/DisplayP3-v2-micro.icc', 'DisplayP3-v2-micro.icc')

run_command(copy, 'test/pathtest.cpp', 'pathtest.cpp')
run_command(copy, 'test/data/case.snitch', 'data/case.snitch')
run_command(copy, 'test/data/kokopaska2.zip', 'data/kokopaska2.zip')
run_command(copy, 'test/data/kokopaska.zip', 'data/kokopaska.zip')
run_command(copy, 'test/data/outer.zip', 'data/outer.zip')
run_command(copy, 'test/data/foo/test.txt', 'data/foo/test.txt')
run_command(copy, 'test/data/fake/random.rar', 'data/fake/random.rar')
run_command(copy, 'test/data/fake/random.snitch', 'data/fake/random.snitch')
run_command(copy, 'test/data/fake/random.zip', 'data/fake/random.zip')
run_command(copy, 'test/data/bad.xxx/dummy.txt', 'data/bad.xxx/dummy.txt')
run_command(copy, 'test/data/bad.zip/dummy.txt', 'data/bad.zip/dummy.txt')
