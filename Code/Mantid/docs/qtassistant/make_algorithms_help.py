#!/usr/bin/env python
import argparse
from lxml import etree as le # python-lxml on rpm based systems
import lxml.html
from lxml.html import builder as lh
import os
from string import split,join
import sys

OUTPUTDIR = "generated"
WEB_BASE  = "http://www.mantidproject.org/"

class QHPFile:
    def __init__(self, namespace, folder="doc"):
        self.__root = le.Element("QtHelpProject",
                                 **{"version":"1.0"})

        self.__addtxtele(self.__root, "namespace", namespace)
        self.__addtxtele(self.__root, "virtualFolder", folder)

        self.__filterSect = le.SubElement(self.__root, "filterSection")

        self.__keywordsEle = None # should have 'keywords' section
        self.__keywords = []
        self.__filesEle = None # should have 'files' section
        self.__files = []

    def addTOC(self, contents):
        """
        The contents must all be specified at once. They should be in the
        form of [(title1, ref1), (title2, ref2), ...].
        """
        toc = le.SubElement(self.__filterSect, "toc")
        for (title, ref) in contents:
            le.SubElement(toc, "section",
                          **{"title":title, "ref":ref})

    def addFile(self, filename, keyword=None):
        if not keyword is None:
            if self.__keywordsEle is None:
                self.__keywordsEle = le.SubElement(self.__filterSect, "keywords")
            if not keyword in self.__keywords:
                le.SubElement(self.__keywordsEle, "keyword",
                              **{"name":keyword, "ref":os.path.split(filename)[1]})
                self.__keywords.append(keyword)
        if self.__filesEle is None:
            self.__filesEle = le.SubElement(self.__filterSect, "files")
        if not filename in self.__files:
            self.__addtxtele(self.__filesEle, "file", filename)
            self.__files.append(filename)

    def __addtxtele(self, parent, tag, text):
        ele = le.SubElement(parent, tag)
        ele.text = text
        return ele

    def __str__(self):
        return le.tostring(self.__root, pretty_print=True)#,
    #xml_declaration=True)

    def write(self, filename):
        """
        Write the help configuration to the given filename.
        """
        handle = open(filename, 'w')
        handle.write(str(self))

def addWikiDir(helpsrcdir):
    """
    Add the location of the wiki tools to the python path.
    """
    wikitoolsloc = os.path.join(helpsrcdir, "../../Build")
    wikitoolsloc = os.path.abspath(wikitoolsloc)
    sys.path.append(wikitoolsloc)

def genCatElement(category):
    category = category.split('/')
    text = "<li>"
    filename = 'AlgoCat_' + category[0] + '.html'
    url = filename
    if len(category) > 1:
        text += '/'.join(category[:-1]) + '/'
        url += '#' + '_'.join(category[1:])
    text += '<a href="%s">%s</a></li>' % (url, category[-1])
    return lxml.html.fragment_fromstring(text)

def genAlgoElement(name, versions):
    text = '<a href="%s">%s' % (WEB_BASE+name, name)
    if len(versions) > 1:
        text += ' v' + str(versions[-1])
    text += '</a>'
    return lxml.html.fragment_fromstring(text)

def processCategories(categories, qhp, outputdir):
    # determine the list of html pages
    grouped_categories = {}
    for key in categories.keys():
        shortkey = key.split('/')[0]
        if not shortkey in grouped_categories:
            grouped_categories[shortkey] = []
        grouped_categories[shortkey].append(key)
    pages = grouped_categories.keys()
    pages.sort()

    for page_name in pages:

        root = le.Element("html")
        head = le.SubElement(root, "head")
        head.append(lh.META(lh.TITLE(page_name + " Algorithm Category")))
        body = le.SubElement(root, "body")
        body.append(lh.CENTER(lh.H1(page_name)))

        subcategories = grouped_categories[page_name]
        subcategories.sort()
        for subcategory in subcategories:
            anchor = subcategory.split('/')
            anchor = '_'.join(anchor[1:])
            temp = le.SubElement(body, "h2")
            le.SubElement(temp, 'a', **{"name":anchor})
            temp.text=subcategory
            #body.append(lh.H2(subcategory))
            ul = le.SubElement(body, "ul")
            for (name, versions) in categories[subcategory]:
                li = le.SubElement(ul, "li")
                li.append(genAlgoElement(name, versions))

        filename = "AlgoCat_%s.html" % page_name
        qhp.addFile(filename, page_name+" Algorithm Category")
        filename = os.path.join(outputdir, filename)
        handle = open(filename, 'w')
        handle.write(le.tostring(root, pretty_print=True, xml_declaration=False))

def process(algos, qhp, outputdir):
    from MantidFramework import mtd

    # sort algorithms into categories
    categories = {}
    for (name, versions) in algos:
        alg = mtd.createAlgorithm(name, versions[-1])
        for category in alg.categories():
            category = category.replace('\\', '/')
            if not categories.has_key(category):
                categories[category] = []
            categories[category].append((name, versions))
    categories_list = categories.keys()
    categories_list.sort()

    # sort algorithms into letter based groups
    letter_groups = {}
    for (name, versions) in algos:
        letter = str(name)[0].upper()
        if not letter_groups.has_key(letter):
            letter_groups[letter] = []
        letter_groups[letter].append((str(name), versions))

    ##### put together the top of the html document
    root = le.Element("html")
    head = le.SubElement(root, "head")
    head.append(lh.META(lh.TITLE("Algorithms Index")))
    body = le.SubElement(root, "body")
    body.append(lh.CENTER(lh.H1("Algorithms Index")))

    ##### section for categories 
    div_cat = le.SubElement(body, "div", **{"id":"alg_cats"})
    div_cat.append(lh.H2("Subcategories"))
    above = None
    ul = le.SubElement(div_cat, "ul")
    for category in categories_list:
        ul.append(genCatElement(category))

    ##### section for alphabetical 
    div_alpha = le.SubElement(body, "div", **{"id":"alg_alpha"})
    div_alpha.append(lh.H2("Alphabetical"))

    letters = letter_groups.keys()
    letters.sort()

    # print an index within the page
    para_text = "<p>"
    for letter in map(chr, range(65, 91)):
        if letter in letters:
            para_text += "<a href='#algo%s'>%s</a> " % (letter, letter)
        else:
            para_text += letter + ' '
    para_text += "</p>"
    div_alpha.append( lxml.html.fragment_fromstring(para_text))

    # print the list of algorithms by name
    for letter in letters:
        temp = le.SubElement(div_alpha, 'h3')
        le.SubElement(temp, 'a', **{"name":'algo'+letter})
        temp.text = letter

        ul = le.SubElement(div_alpha, "ul")
        for (name, versions) in letter_groups[letter]:
            li = le.SubElement(ul, "li")
            li.append(genAlgoElement(name, versions))

    # print an index within the page
    div_alpha.append( lxml.html.fragment_fromstring(para_text))

    filename = os.path.join(outputdir, "algorithms_index.html")
    handle = open(filename, 'w')
    handle.write(le.tostring(root, pretty_print=True, xml_declaration=False))

    shortname = os.path.split(filename)[1]
    qhp.addFile(shortname, "Algorithms Index")

    # create all of the category pages
    processCategories(categories, qhp, outputdir)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate qtassistant docs " \
                  + "for the algorithms")
    defaultmantidpath = ""
    parser.add_argument('-m', '--mantidpath', dest='mantidpath',
                        default=defaultmantidpath,
                        help="Full path to the Mantid compiled binary folder. Default: '%s'. This will be saved to an .ini file" % defaultmantidpath)

    args = parser.parse_args()

    # where to put the generated files
    helpsrcdir = os.path.dirname(os.path.abspath(__file__))
    helpoutdir = os.path.join(helpsrcdir, OUTPUTDIR)
    print helpoutdir
    addWikiDir(helpsrcdir)

    # initialize mantid
    import wiki_tools
    wiki_tools.initialize_Mantid(args.mantidpath)
    algos = wiki_tools.get_all_algorithms(True)

    # setup the qhp file
    qhp = QHPFile("org.mantidproject.algorithms")

    process(algos, qhp, helpoutdir)
    qhp.write(os.path.join(helpoutdir, "algorithms.qhp"))
