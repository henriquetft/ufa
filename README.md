# UFA (User File Attributes)

**UFA** is a user-level tool that enables setting and managing custom **tags** and **key-value attributes** on regular files. It doesn’t rely on any specific filesystem — all metadata is stored separately in a lightweight SQLite database.

With UFA, you can:
- Organize files using searchable metadata (tags and attributes),
- Search files by any combination of tags or attributes,
- Mount a **tag-based virtual filesystem** via FUSE for alternative 
  navigation.

Because metadata is stored externally, UFA is compatible with **any filesystem** and **any OS** where you can run the daemon and CLI tools.

---

## 🚀 Key Features

- Add custom metadata (e.g. `author=me`, `year=2024`)
- Search files by tag and attribute combinations
- Tag-based virtual filesystem (FUSE)
- Lightweight and local: no server or root required
- Compatible with any filesystem (ext4, NTFS, FAT32, etc.)
- Integration with Nautilus
- Modular CLI tools and background daemon

---

## 🔧 Example Usage

```bash
ufaattr set document.pdf author me         # Set an attribute
ufaattr get document.pdf author            # Get attribute value
ufatag set book.pdf programming            # Add a tag
ufatag list book.pdf                       # List all tags

ufafind -t programming -t unix -a year=2023  # Search files
```

Repositories can be initialized with:

```bash
ufactl init .
```

Then added to global configuration:

```bash
ufactl add .
```

Once added, it can search for files across all registered repositories.

---

## 🛠 Command-Line Tools

### `ufactl`
Manage repositories:
- `init`: Initialize a new repository
- `add`: Add repository to global configuration
- `list`: List registered repositories
- `remove`: Remove repository from configuration

### `ufaattr`
Manage file attributes:
- `set <file> <key> <value>`
- `get <file> <key>`

### `ufatag`
Manage file tags:
- `set <file> <tag>`
- `list <file>`

### `ufafind`
Search files using filters:
- Tags: `-t <tag>`
- Attributes: `-a <key>=<value>`

### `ufafs`
Mount a virtual filesystem organized by tags:

```bash
ufafs -f -s --repository=/home/user/myrepo /home/user/tags_fs
```

---

## 🧩 Integration with File Managers

You can integrate UFA into graphical file managers.

- **Nautilus** extension is available under the `contrib/` directory.
- These extensions provide context menus for managing tags and attributes.

---

## 🧠 Architecture & Design

UFA uses a **client-daemon architecture**. CLI tools communicate with a background daemon (`ufad`) via a **Unix domain socket**, using structured **JSON messages**.

This provides:

- Clear API boundaries (e.g., for GUIs, web UIs or remote tools)
- High extensibility

All metadata is stored in an SQLite database (`repo.db`) at the repository root.
This design makes UFA:

- **Cross-platform**: No reliance on extended file attributes (xattr)
- **Portable**: Metadata moves with your folder
- **Versionable**: Easily tracked with Git or backups
- **Scalable**: Structured queries enable fast filtering/search

---

## 🏗 Build & Install

Recommended installation in your `$HOME`:

```bash
git clone https://github.com/henriquetft/ufa.git
cd ufa
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=$HOME/.local ..
make
make install
```

Start the daemon:

```bash
systemctl --user start ufad.service
```

(Optional) Enable it on login:

```bash
systemctl --user enable ufad.service
```

---

## 📝 License

UFA is licensed under the **BSD 3-Clause License**.

---

## 🙌 Contributing & Ideas

UFA was designed to be modular and extensible. Ideas for future development:

- GUI management tool
- Dolphin extension (context menu)
- Integration with any other file manager
- Enhance search capabilities (additional operators, etc)
