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

Creates context menus helping user to manage tags.

Creates a context menu on right-click of files within a repository
that opens a dialog allowing the user to set and unset tags.

A context menu on right-click of a repository folder allowing user
to create a tag.
"""

import subprocess
import shutil
import os.path
from urllib.parse import unquote
from subprocess import Popen, TimeoutExpired
from gi.repository import Nautilus, GObject


def get_ufa_repo_by_file(filepath):
    """ Reads .ufarepo file and returns root repository """
    return get_ufa_repo_by_dir("/".join(filepath.split('/')[:-1]))


def get_ufa_repo_by_dir(filepath):
    """ Reads .ufarepo file and returns root repository """
    path_list = filepath.split('/')
    path_list.append('.ufarepo')
    ufarepo_path = "/".join(path_list)
    try:
        with open(ufarepo_path, "r") as f:
            return f.readline().strip()
    except FileNotFoundError:
        return None


def is_repo_dir(filepath):
    """ Returns whether the filepath is a repository path """
    filepath += "/.ufarepo"
    log(f"checking file: {filepath}")
    return os.path.isfile(filepath)


def log(message):
    """ Logs message to the console """
    print(message)


class UFACommand:
    """ Execute UFA command-line utilities """

    def __init__(self, ufatag_app, repository):
        self.ufatag = ufatag_app
        self.repository = repository

    @classmethod
    def execute(cls, s, exp=[0]):
        """ Executes a shell command """
        command = s.split(" ")[0]
        with Popen(s, shell=True, stdout=subprocess.PIPE) as proc:
            try:
                log(f'\nExecuting command:\n{s}')
                stdout = proc.communicate()[0]
                if proc.returncode not in exp:
                    raise Exception("Error on {}. return code: {}".format(
                        command, proc.returncode))
                stdout = stdout.decode('utf-8')
                log(f'\nCommand return stdout:\n{stdout}')
                log(f'\nCommand returncode:\n{proc.returncode}')
                return stdout
            except TimeoutExpired:
                proc.kill()
                raise

    def get_all_tags(self):
        """ Executes ufatag list-all """
        stdout = self.execute(
            "{} -r {} list-all".format(self.ufatag, self.repository))
        list_tags = stdout.split('\n')[:-1]
        return list_tags

    def get_tags_for_file(self, filename):
        """ Executes ufatag list FILE """
        stdout = self.execute(
            "{} -r {} list '{}'".format(self.ufatag, self.repository,
                                        filename))
        list_tags = stdout.split('\n')[:-1]
        return list_tags

    def set_tags_for_file(self, filename, tag_list):
        """ Executes ufatag set FILE tag """
        for tag in tag_list:
            self.execute(
                "{} -r {} set '{}' {}".format(self.ufatag,
                                              self.repository,
                                              filename,
                                              tag))

    def unset_tags_for_file(self, filename, tag_list):
        """ Executes ufatag unset FILE tag """
        for tag in tag_list:
            self.execute(
                "{} -r {} unset '{}' {}".format(self.ufatag,
                                                self.repository,
                                                filename,
                                                tag))

    def create_tag(self, tagname):
        self.execute("{} -r {} create {}".format(self.ufatag,
                                                 self.repository,
                                                 tagname))


class Command(UFACommand):
    def dialog_tags(self, filename, all_tags, tags_for_file):
        dict_tags_file = {tag: tag in tags_for_file for tag in all_tags}
        tags_param = ' '.join("{} {}".format(str(val).upper(), key)
                              for (key, val) in dict_tags_file.items())
        stdout = self.execute("""zenity --list --text 'UFA tags for {}' \
                                 --checklist \
                                 --column 'Pick' --column 'Tags' \
                                 --title 'Choose tags for this file' \
                                 --width=300 --height=350 {} \
                                 --separator=':'"""
                              .format(filename, tags_param),
                              exp=[0])
        tags_checked = [x.strip() for x in stdout.split(':')]
        if len(tags_checked) == 0 or \
                len(tags_checked) == 1 and \
                tags_checked[0].strip() == '':
            return []
        return tags_checked

    def dialog_new_tag(self):
        stdout = self.execute("""zenity --entry --title='Add new tag' \
            --text='Enter name of new tag:'""", exp=[0, 1])
        return stdout

    def dialog_info(self, message):
        self.execute(f"""zenity --info --title='UFA' --width=200 \
                        --text='{message}'""")


class MenuProvider(GObject.GObject, Nautilus.MenuProvider):
    def __init__(self):
        self.ufatag_app = shutil.which("ufatag")

    def menu_activate_manage_tags(self, menu, ufarepo, filepath):
        log(f"Calling manage_tags: repo={ufarepo}, filepath={filepath}")
        filename = filepath.split('/')[-1:][0]
        log(f"Filename: {filename}")

        cmd = Command(self.ufatag_app, ufarepo)

        all_tags = cmd.get_all_tags()
        tags_for_file = cmd.get_tags_for_file(filename)

        try:
            tags_checked = cmd.dialog_tags(filename, all_tags, tags_for_file)

            log(f"Tags checked: {tags_checked}")
            tags_to_unset = set(tags_for_file) - set(tags_checked)
            tags_to_set = set(tags_checked) - set(tags_for_file)

            log("Executing sets and unsets commands ...")
            cmd.set_tags_for_file(filename, tags_to_set)
            cmd.unset_tags_for_file(filename, tags_to_unset)
        except Exception:
            log("retuned non-zero. error or operation canceled")

    def menu_activate_create_tags(self, menu, ufarepo):
        log(f"Calling create_tags: repo={ufarepo}")
        cmd = Command(self.ufatag_app, ufarepo)
        tag_name = cmd.dialog_new_tag()
        tag_name = tag_name.strip().replace(" ", "-")
        if tag_name:
            log(f"Checking whether a tag exists: {tag_name}")
            all_tags = cmd.get_all_tags()
            if tag_name in all_tags:
                cmd.dialog_info(f"Tag {tag_name} already exist")
            else:
                log(f"Creating tag: {tag_name}")
                cmd.create_tag(tag_name)

    def get_file_items(self, window, files):
        if self.ufatag_app is None:
            log("ufatag not found")
            return None
        if len(files) != 1:
            return None
        file = files[0]
        if file.is_directory() or file.get_uri_scheme() != 'file':
            return None

        log("\n------------------------------------------------------------\n")
        filepath = str(file.get_uri())
        filepath = filepath.split("file://")[1]
        filepath = unquote(filepath)
        ufarepo = get_ufa_repo_by_file(filepath)
        if ufarepo is None:
            log(f"{filepath} is not in a repo dir")
            return None
        log(f"Repository path: {ufarepo}")
        log(f"File path: {filepath}")

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

        sub_menuitem.connect('activate',
                             self.menu_activate_manage_tags,
                             ufarepo,
                             filepath)
        return (top_menuitem,)

    def get_background_items(self, window, file):
        if self.ufatag_app is None:
            log("ufatag not found")
            return None

        if not file.is_directory() or file.get_uri_scheme() != 'file':
            return None

        log("\n------------------------------------------------------------\n")
        filepath = str(file.get_uri())
        filepath = filepath.split("file://")[1]
        filepath = unquote(filepath)
        if not is_repo_dir(filepath):
            return None
        repo_dir = get_ufa_repo_by_dir(filepath)
        log("REPO DIR: f{repo_dir}")
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
                             repo_dir)

        return (menuitem,)
