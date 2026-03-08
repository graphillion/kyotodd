project = 'KyotoDD'
copyright = '2026, KyotoDD Contributors'
author = 'KyotoDD Contributors'
release = '0.1.0'

extensions = [
    'breathe',
]

# -- Breathe Configuration --
breathe_projects = {
    'KyotoDD': '_doxygen/xml',
}
breathe_default_project = 'KyotoDD'
breathe_default_members = ('members',)

# -- Theme --
html_theme = 'furo'

# -- i18n preparation --
locale_dirs = ['locale/']
gettext_compact = False

# -- HTML --
html_static_path = ['_static']
templates_path = ['_templates']

# -- Exclude patterns --
exclude_patterns = ['_build', '_doxygen']
