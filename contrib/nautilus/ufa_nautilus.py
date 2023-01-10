#!/usr/bin/env python

# Copyright (c) 2021, Henrique Teófilo
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#     1. Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#     2. Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in the
#        documentation and/or other materials provided with the distribution.
#
#     3. Neither the name of the copyright holder nor the
#        names of its contributors may be used to endorse or promote products
#        derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER NOR THE NAMES OF ITS
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""
UFA Nautilus integration.

Creates a context menu to help user to manage tags and attributes.
using UFA.

A context menu on right-click of a repository folder allowing user
to manage tags and attributes
"""

__author__ = "Henrique Teofilo <henrique at teofilo.me>"
__version__ = "1.0"
__appname__ = "ufa-nautilus"
__app_disp_name__ = "UFA Nautilus"
__website__ = "https://github.com/henriquetft/ufa"

import subprocess
import traceback
import shlex
import sys
import shutil
import os.path
from urllib.parse import unquote
from subprocess import Popen, TimeoutExpired
from gi.repository import Nautilus, GObject


def log(message):
    """ Log message to the console """
    print(message)


def get_ufa_repo_by_file(filepath):
    """ Reads .ufarepo file and returns root repository """
    return get_ufa_repo_by_dir("/".join(filepath.split('/')[:-1]))


def get_ufa_repo_by_dir(filepath):
    """ Reads .ufarepo file and returns root repository """
    path_list = filepath.split('/')
    path_list.append('.ufarepo')
    ufa_repo = "/".join(path_list)
    try:
        with open(ufa_repo, "r") as f:
            return f.readline().strip()
    except FileNotFoundError:
        return None


def is_repo_dir(filepath):
    """ Returns whether the filepath is a repository path """
    filepath += "/.ufarepo"
    log(f"checking file: {filepath}")
    return os.path.isfile(filepath)


def error_handler(func):
    """ Decorator to handle exception on menu """
    def wrapper(*args, **kwargs):
        """ Decorator wrapper function """
        try:
            func(*args, **kwargs)
        except Exception:
            print(f"Error on {func.__name__}")
            traceback.print_exception(*sys.exc_info())
            Commands.dialog_error("Error")
    return wrapper


class Commands:
    """ Shell commands """
    ZENITY = shutil.which("zenity")

    @classmethod
    def is_ok(cls):
        """ Checks whether CLI app is found """
        return cls.ZENITY is not None

    @classmethod
    def execute(cls, cmd, exp=[0], returncode=False):
        """ Executes a shell command """
        command = cmd.split(" ")[0]
        with Popen(cmd, shell=True, stdout=subprocess.PIPE) as proc:
            try:
                log(f'\nExecuting command:\n{cmd}')
                stdout = proc.communicate()[0]
                if proc.returncode not in exp:
                    raise Exception("Error on {}. return code: {}".format(
                        command, proc.returncode))
                stdout = stdout.decode('utf-8')
                log(f'\nCommand return stdout:\n{stdout}')
                log(f'\nCommand returncode:\n{proc.returncode}')
                if returncode:
                    return stdout, proc.returncode
                return stdout
            except TimeoutExpired:
                proc.kill()
                raise

    @classmethod
    def dialog_tags(cls, filename, all_tags, tags_for_file):
        dict_tags_file = {tag: tag in tags_for_file for tag in all_tags}
        tags_param = ' '.join("{} {}".format(str(val).upper(), key)
                              for (key, val) in dict_tags_file.items())
        stdout, ret = cls.execute("""{} --list --text {} \
                                  --checklist \
                                  --column 'Pick' --column 'Tags' \
                                  --title 'Choose tags for this file' \
                                  --width=300 --height=350 {} \
                                  --separator=':'""".format(cls.ZENITY, shlex.quote(filename),
                                                            tags_param),
                                  exp=[0, 1], returncode=True)
        if ret == 1:  # when the user cancels operation
            return None
        tags_checked = [x.strip() for x in stdout.split(':')]
        if len(tags_checked) == 0 or \
                len(tags_checked) == 1 and \
                tags_checked[0].strip() == '':
            return []
        return tags_checked

    @classmethod
    def dialog_new_tag(cls):
        stdout = cls.execute("""{} --entry --title='Add new tag' \
            --text='Enter name of new tag:'""".format(cls.ZENITY), exp=[0, 1])
        return stdout

    @classmethod
    def dialog_info(cls, message):
        cls.execute(f"""{cls.ZENITY} --info --title='UFA' --width=200 \
                        --text='{message}'""")

    @classmethod
    def dialog_error(cls, message):
        cls.execute(f"""{cls.ZENITY} --error --title='UFA' --width=200 \
                        --text='{message}'""")

    @classmethod
    def dialog_set_attr(cls, filename):
        stdout = cls.execute(f"""{cls.ZENITY} --forms --title="Set attribute" \
                              --text="Enter information about attribute" \
                              --separator="||||" \
                              --add-entry="Attribute name" \
                              --add-entry="Value" """, exp=[0, 1])
        if not stdout:
            return None
        return tuple(stdout.split("||||"))

    @classmethod
    def dialog_edit_attr(cls, filename, attr, value):
        stdout, retcode = cls.execute("""{} --entry --title='Edit attribute {}' \
                              --text='Enter new value:' \
                              --entry-text "{}" """.format(cls.ZENITY, attr, value),
                             exp=[0, 1],
                             returncode=True)
        if retcode == 1:
            return None
        return stdout

    @classmethod
    def dialog_list_attrs(cls, filename, attrs):
        """ Receives filename and a list of tuples attribute,value """
        attrs_str = " ".join(["\"" + str(x[0]).strip() + '" "'
                              + str(x[1]).strip() + "\"" for x in attrs])
        stdout = cls.execute("""{} --list \
                              --title="Choose the attribute to edit" \
                              --column="Attribute" --column="Value" \
                              {}""".format(cls.ZENITY, attrs_str), exp=[0, 1])
        # remove last '\n'
        return "".join(stdout.split('\n')[:-1])


class UFACommand:
    """ Execute UFA command-line utilities """

    def __init__(self, ufatag, ufaattr):
        self.ufatag = ufatag
        self.ufaattr = ufaattr

    def is_ok(self):
        """ Checks whether CLI app is found """
        return self.ufatag is not None and self.ufaattr is not None

    def get_all_tags(self, repo):
        """ Executes ufatag list-all """
        stdout = Commands.execute(
            "{} -l off -r {} list-all".format(self.ufatag, repo))
        list_tags = stdout.split('\n')[:-1]
        return list_tags

    def get_tags_for_file(self, repo, filepath):
        """ Executes ufatag list FILE """
        stdout = Commands.execute(
            "{} -l off list {}".format(self.ufatag,
                                       shlex.quote(filepath)))
        list_tags = stdout.split('\n')[:-1]
        return list_tags

    def set_tags_for_file(self, repo, filepath, tag_list):
        """ Executes ufatag set FILE tag """
        for tag in tag_list:
            Commands.execute(
                "{} -l off set {} {}".format(self.ufatag,
                                             shlex.quote(filepath),
                                             tag))

    def unset_tags_for_file(self, repo, filepath, tag_list):
        """ Executes ufatag unset FILE tag """
        for tag in tag_list:
            Commands.execute(
                "{} -l off unset {} {}".format(self.ufatag,
                                               shlex.quote(filepath),
                                               tag))

    def create_tag(self, repo, tagname):
        """ Executes ufatag create TAG """
        Commands.execute("{} -l off -r {} create {}".format(self.ufatag,
                                                     shlex.quote(repo),
                                                     tagname))

    def get_attrs_for_file(self, repo, filepath):
        """ Executes ufaattr describe FILE """
        stdout = Commands.execute(
            "{} -l off describe {}".format(self.ufaattr,
                                           shlex.quote(filepath)))
        list_attrs = stdout.split('\n')[:-1]

        return list_attrs

    def get_attr_value_for_file(self, repo, filepath, attribute):
        """ Executes ufaattr get FILE ATTRIBUTE """
        stdout = Commands.execute(
            "{} -l off get {} {}".format(self.ufaattr,
                                         shlex.quote(filepath),
                                         attribute))
        return stdout.strip()

    def set_attr_value_for_file(self, repo, filepath, attr, value):
        """ Executes ufaattr set FILE ATTR VALUE """
        Commands.execute("{} -l off set {} {} {}".format(self.ufaattr,
                                                         shlex.quote(filepath),
                                                         attr,
                                                         value))


class MenuProvider(GObject.GObject, Nautilus.MenuProvider):

    def __init__(self):
        ufatag = shutil.which("ufatag")
        ufaattr = shutil.which("ufaattr")
        log(f"UFATAG CLI found.....: {ufatag}")
        log(f"UFAATTR CLI found....: {ufaattr}")
        self.cmd = UFACommand(ufatag, ufaattr)

    def is_cli_ok(self):
        if not self.cmd.is_ok():
            log("UFA command line utilities not found no PATH")
            return False
        if not Commands.is_ok():
            log("zenit not found on PATH")
        return True

    @error_handler
    def menu_activate_manage_tags(self, menu, repo, filename):
        log(f"Calling manage_tags: repo={repo}, filepath={filename}")
        # presenting file tags
        all_tags = self.cmd.get_all_tags(repo)
        tags_for_file = self.cmd.get_tags_for_file(repo, filename)
        tags_checked = Commands.dialog_tags(filename, all_tags, tags_for_file)
        if tags_checked is None:
            log("Do not change tags!")
        else:
            log(f"Tags checked: {tags_checked}")
            tags_to_unset = set(tags_for_file) - set(tags_checked)
            tags_to_set = set(tags_checked) - set(tags_for_file)
            log("Executing set and unset commands ...")
            self.cmd.set_tags_for_file(repo, filename, tags_to_set)
            self.cmd.unset_tags_for_file(repo, filename, tags_to_unset)

    @error_handler
    def menu_activate_list_attrs(self, menu, repo, filename):
        log(f"Calling list_attrs: repo={repo}, filepath={filename}")
        ret = self.cmd.get_attrs_for_file(repo, filename)
        arg = list(map(lambda x: tuple(x.split('\t')), ret))
        # choose an attribute to edit
        attr_to_edit = Commands.dialog_list_attrs(filename, arg)
        if attr_to_edit:
            attr_value = self.cmd.get_attr_value_for_file(repo, filename, attr_to_edit)
            new_value = Commands.dialog_edit_attr(filename, attr_to_edit, attr_value)
            if not new_value is None:
                self.cmd.set_attr_value_for_file(repo, filename, attr_to_edit, new_value)

    @error_handler
    def menu_activate_add_attrs(self, menu, repo, filename):
        log(f"Calling add_attrs: repo={repo}, filename={filename}")
        attr = Commands.dialog_set_attr(filename)
        if not attr:
            return
        self.cmd.set_attr_value_for_file(repo, filename, attr[0], attr[1])

    @error_handler
    def menu_activate_create_tags(self, menu, repo):
        log(f"Calling create_tags: repo={repo}")
        tag_name = Commands.dialog_new_tag()
        tag_name = tag_name.strip().replace(" ", "-")
        if tag_name:
            log(f"Checking whether a tag exists: {tag_name}")
            all_tags = self.cmd.get_all_tags(repo)
            if tag_name in all_tags:
                Commands.dialog_info(f"Tag {tag_name} already exist")
            else:
                log(f"Creating tag: {tag_name}")
                self.cmd.create_tag(repo, tag_name)

    def get_file_items(self, window, files):
        """ Returns the list of Nautilus.MenuItem """
        log("Creating menu for file")
        if not self.is_cli_ok():
            return None
        if len(files) != 1:
            return None
        file = files[0]
        if file.is_directory() or file.get_uri_scheme() != 'file':
            return None

        filepath = str(file.get_uri())
        filepath = filepath.split("file://")[1]
        filepath = unquote(filepath)
        repo = get_ufa_repo_by_file(filepath)
        if repo is None:
            log(f"{filepath} is not in a repo dir")
            return None
        log(f"Repository path: {repo}")
        log(f"File path: {filepath}")
        #filename = filepath.split('/')[-1:][0]

        top_menuitem = Nautilus.MenuItem(name='MenuProvider::UFA',
                                         label='UFA',
                                         tip='',
                                         icon='')

        submenu = Nautilus.Menu()
        top_menuitem.set_submenu(submenu)

        sub_menuitem = Nautilus.MenuItem(name='MenuProvider::Manage_Tags',
                                         label='Manage Tags',
                                         tip='',
                                         icon='')
        submenu.append_item(sub_menuitem)

        sub_list_attr = Nautilus.MenuItem(name='MenuProvider::List_Attr',
                                          label='List Attributes',
                                          tip='',
                                          icon='')
        submenu.append_item(sub_list_attr)

        sub_add_attr = Nautilus.MenuItem(name='MenuProvider::Add_Attr',
                                         label='Add Attribute',
                                         tip='',
                                         icon='')
        submenu.append_item(sub_add_attr)

        sub_menuitem.connect('activate',
                             self.menu_activate_manage_tags,
                             repo,
                             filepath)
        sub_list_attr.connect('activate',
                              self.menu_activate_list_attrs,
                              repo,
                              filepath)
        sub_add_attr.connect('activate',
                             self.menu_activate_add_attrs,
                             repo,
                             filepath)
        log("Menu created\n")
        return (top_menuitem,)

    def get_background_items(self, window, file):
        log("Creating menu for folder")
        if not self.is_cli_ok():
            return None

        if not file.is_directory() or file.get_uri_scheme() != 'file':
            return None

        filepath = str(file.get_uri())
        filepath = filepath.split("file://")[1]
        filepath = unquote(filepath)
        if not is_repo_dir(filepath):
            return None
        repo = get_ufa_repo_by_dir(filepath)
        log(f"REPO DIR: {repo}")
        submenu = Nautilus.Menu()
        submenu_item = Nautilus.MenuItem(name='MenuProvider::Create_Tag',
                                         label='Create tag',
                                         tip='',
                                         icon='')
        submenu.append_item(submenu_item)

        menuitem = Nautilus.MenuItem(name='MenuProvider::UFA_Options',
                                     label='UFA',
                                     tip='',
                                     icon='')
        menuitem.set_submenu(submenu)

        submenu_item.connect('activate',
                             self.menu_activate_create_tags,
                             repo)
        log("Menu created\n")
        return (menuitem,)
