import os
import os.path
from doxygen_builder import *

def addSources(env, sources):
	env['MY_SOURCES'] += [os.path.join(Dir('.').abspath, s) for s in sources]

def addHeaders(env, headers):
	env['HEADERS'] += [os.path.join(Dir('.').srcnode().abspath, h) for h in headers]

def getCacheLineSize():
	index = 0
	cache_line_size = 0
	while cache_line_size == 0 and os.path.exists('/sys/devices/system/cpu/cpu0/cache/index' + str(index)):
		with open('/sys/devices/system/cpu/cpu0/cache/index' + str(index) + '/coherency_line_size', 'r') as f:
			cache_line_size = int(f.read())

	if cache_line_size == 0:
		print \
"""You decided to optimize for speed, but I can't read your cache line size from
/sys/devices/system/cpu/cpu0/cache/index*. Please tell me what it is, in bytes,
without the 'b' or 'B' suffix. If you don't know it, try 32 or 64 and see which
is faster.
 """,
		cache_line_size = int(raw_input("Enter your cache line size: "))
	return cache_line_size

relIncludePaths = Split("""src
						   src/debuggers
						   src/expression_compilers/ascii
						   src/expression_compilers/string
						   src/expression_compilers/unicode
						   src/input/ascii
						   src/input/unicode
						   src/instruction_sets/ascii
						   src/instruction_sets/standard
						   src/instruction_sets/image
						   src/instruction_sets/unicode
						   src/kernel
						   src/tree_compilers/ascii
						   src/tree_compilers/standard
						   src/tree_compilers/string
						   src/tree_compilers/unicode""")

# the include paths have to be explicitly rooted or scons passes
# them to gcc as though they were rooted in the 'build' directory
includePaths = [Dir(p).abspath for p in relIncludePaths]

AddOption('--prefix',
		  dest='prefix',
		  type='string',
		  nargs=1,
		  action='store',
		  metavar='DIR',
		  default='/usr/local',
		  help='installation prefix')

vars = Variables()
vars.Add(EnumVariable('ARCH', 'integer siz/sys/devices/system/cpu/cpu0/cache/indexe specification', '',
					  allowed_values = ('', 'unix32', 'unix64', 'win32', 'win64', 'win3.1', 'mac',
										'lp64', 'llp64', 'ilp32', 'lp32')))

vars.Add(EnumVariable('OPTIMIZE', 'optimization preference', '',
					  allowed_values = ('', 'size', 'size_const_clist', 'speed')))

VariantDir('build', 'src')
env = Environment(variables = vars,
				  CPPPATH = includePaths,
				  PREFIX = GetOption('prefix'),
				  ENV = os.environ,
				  ARCH = '${ARCH}',
				  OPTIMIZE = '${OPTIMIZE}',
				  MY_SOURCES = [],
				  HEADERS = [])

conf = Configure(env)
env['HAS_STDINT'] = conf.CheckCHeader('stdint.h')
env['HAS_ICU'] = conf.CheckLibWithHeader(['icui18n'], ['unicode/ustring.h'], 'c') and conf.CheckLibWithHeader(['icuio'], ['unicode/ustdio.h'], 'c')
env['HAS_ANTLR'] = conf.CheckLibWithHeader(['antlr3c'], ['antlr3.h'], 'c')

if int(ARGUMENTS.get('debug', 1)):
	print "Including debugging symbols and disabling compiler optimizations"
	env.Append(CCFLAGS = ['-Wall', '-ggdb', '-O0'])
else:
	print "Not including debugging symbols and enabled compiler optimizations"
	env.Append(CCFLAGS = ['-O3', '-DNDEBUG'])

if env['OPTIMIZE'] == '' or env['OPTIMIZE'] == 'size':
	print "Optimizing for size"
	env.Append(CCFLAGS = ['-DOPTIMIZE_FOR_SIZE'])
elif env['OPTIMIZE'] == 'size':
	print 'Optimizing for size w/ constant compoundlist size'
	env.Append(CCFLAGS = ['-DOPTIMIZE_FOR_SIZE', '-DCONST_CLIST_SIZE'])
else:
	print "Optimizing for speed"
	cache_line_size = getCacheLineSize()
	print "Using " + str(cache_line_size) + " byte cache line size"
	env.Append(CCFLAGS = ['-DOPTIMIZE_FOR_SPEED', '-DCACHE_LINE_SIZE=' + str(cache_line_size)])

env.AddMethod(addHeaders, 'addHeaders')
env.AddMethod(addSources, 'addSources')

libs = ['Judy']
if env['HAS_ICU']:
	libs += ['icui18n', 'icuio']
if env['HAS_ANTLR']:
	libs += ['antlr3c']

Export(['env', 'conf'])
SConscript('src/SConscript', variant_dir='build')

env.Alias('re', env.SharedLibrary(target = 'lib/reos', source = env['MY_SOURCES'], LIBS = libs))

Default('re')
env.Alias('re', env.Install('include', env['HEADERS']))

env.Alias('install', env.Install('$PREFIX/lib', Glob('lib/*')))
env.Alias('install', env.Install('$PREFIX/include/reos', Glob('include/*')))

generate(env)
env.Alias('doc', env.Doxygen('Doxyfile'))

SConscript('examples/SConscript')
SConscript('tests/SConscript')
