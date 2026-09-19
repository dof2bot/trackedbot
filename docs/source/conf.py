# -*- coding: utf-8 -*-
#
# Configuration file for the Sphinx documentation builder for trackedbot.
#

from __future__ import annotations

import os
import sys

# -- Project information -----------------------------------------------------

project = 'trackedbot'
copyright = '2026, Vladimir Roncevic <elektron.ronca@gmail.com>'
author = 'Vladimir Roncevic <elektron.ronca@gmail.com>'

version = '1.0'
release = '1.0.0'

# -- General configuration ---------------------------------------------------

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.viewcode',
]

# Support Markdown (.md) files if myst_parser is installed
try:
    import myst_parser  # type: ignore
    extensions.append('myst_parser')
    source_suffix = {
        '.rst': 'restructuredtext',
        '.md': 'markdown',
    }
except ImportError:
    source_suffix = '.rst'

templates_path = ['_templates']
master_doc = 'index'
language = 'en'
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']
pygments_style = 'sphinx'

# -- Options for HTML output -------------------------------------------------

html_theme = 'classic'
html_logo = '_static/trackedbot_logo.png'
html_static_path = ['_static']

html_theme_options = {
    'rightsidebar': 'false',
    'stickysidebar': 'true',
    'collapsiblesidebar': 'true',
    'externalrefs': 'true',
    'footerbgcolor': '#0f172a',
    'footertextcolor': '#94a3b8',
    'sidebarbgcolor': '#1e293b',
    'sidebartextcolor': '#e2e8f0',
    'sidebarlinkcolor': '#00d2ff',
    'relbarbgcolor': '#0f172a',
    'relbartextcolor': '#e2e8f0',
    'relbarlinkcolor': '#00d2ff',
    'headbgcolor': '#ffffff',
    'headtextcolor': '#0f172a',
    'headlinkcolor': '#0088ff',
    'linkcolor': '#0088ff',
    'visitedlinkcolor': '#0055aa',
    'codebgcolor': '#f8fafc',
    'codetextcolor': '#0f172a',
    'bodyfont': 'sans-serif',
    'headfont': 'sans-serif',
}

# -- Options for HTMLHelp output ---------------------------------------------

htmlhelp_basename = 'trackedbotdoc'

# -- Options for LaTeX output ------------------------------------------------

latex_elements = {
    'papersize': 'letterpaper',
    'pointsize': '10pt',
}

latex_documents = [
    (master_doc, 'trackedbot.tex', 'trackedbot Documentation',
     'Vladimir Roncevic \\textless{}elektron.ronca@gmail.com\\textgreater{}', 'manual'),
]

# -- Options for manual page output ------------------------------------------

man_pages = [
    (master_doc, 'trackedbot', 'trackedbot Documentation',
     [author], 1)
]

# -- Options for Texinfo output ----------------------------------------------

texinfo_documents = [
    (master_doc, 'trackedbot', 'trackedbot Documentation',
     author, 'trackedbot', 'Autonomous Tracked Mobile Robot Platform',
     'Miscellaneous'),
]

# -- Options for Epub output -------------------------------------------------

epub_title = project
epub_exclude_files = ['search.html']
