# UFA (User File Attributes)

UFA - User File Attributes is a software that allows the user to set and manage custom tags and "key-value" attributes for files.

Tags and attributes can be used as additional metadata of files, allowing several kinds of operations, such as search by tag/attribute and even mount a tag-based filesystem (FUSE), presenting files based on tags, rather than filesystem folders.

The user makes a new ufa repository with `ufactl init`. It sets up a new repository in the specified folder.

There are several commands to work with files in repositories.

Examples:

* `ufaattr set document.pdf author me`: Set attribute 'author' with value 'me' for document.pdf
* `ufaattr get document.pdf author`: Get the value of attribute 'author'
* `ufatag set book.pdf programming`: Set new tag 'programming' for book.pdf
* `ufatag list book.pdf`: List all tags of book.pdf

Tags and attributes can be used to search for files.

For example:

```shell
ufafind -t programming -t unix -a year=2023
``` 

it will search for files with tags 'pogramming', 'unix' and attribute 'year' with value '2023'

All these commands can be executed from the directory of repository, or the user can specify another repository path with `-r` switch.

However, the best option is to add the repository in the global config, so the commands will work without having to specify directory or need to be in the current dir. In this case, the `ufafind` command will search in all repositories from config. This can be achieved with `ufactl add <dir>`


## Command-line tools
### ufactl

Commands:

* `init`: Initialize a repository
* `add`: Add the repository (global config)
* `list`: List current watched repositories
* `remove`: Remove repository directory from watching list

### ufad

The UFA Daemon must be running to `ufafind`, `ufaattr` and `ufatag` work properly.

### ufafs
Mount a tag-based filesystem.

Example:

```bash
ufafs -f -s --repository=/home/user/myrepo /home/user/folder_organized_by_tags
```

### CLI to work with files
* `ufaattr`: CLI tool for managing attributes of files
* `ufatag`: CLI tool for managing tags of files
* `ufafind`: CLI tool for searching files by tags and attributes


## Integrations
It is possible to integrate UFA with file managers, such as `nautilus` and `dolphin`. In `contrib` folder there is a nautilus extension that creates a context menu to help user to manage tags and attributes.

## Build and install
I recommend installing it in $HOME directory:

```shell
git clone https://github.com/henriquetft/ufa.git
cd ufa
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=$HOME/.local ..
make
make install
```

You must start ufad service
```shell
systemctl --user start ufad.service
```


If you want to enable ufad service (so it will start automatically at login):
```shell
systemctl --user enable ufad.service
```




