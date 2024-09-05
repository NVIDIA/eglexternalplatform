project('eglexternalplatform',
  version : '1.2')

pkg = import('pkgconfig')
pkg.generate(filebase: 'eglexternalplatform',
  name: 'EGL External Platform interface',
  description: 'EGL External Platform interface',
  install_dir: get_option('datadir') / 'pkgconfig')

install_headers(
  'interface/eglexternalplatform.h',
  'interface/eglexternalplatformversion.h',
)

eglexternalplatform_dep = declare_dependency(
  include_directories: include_directories('interface'),
)
meson.override_dependency('eglexternalplatform', eglexternalplatform_dep)
