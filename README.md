# Offrestorecon tool
### What is it?
Offrestorecon is a tool to do restorecon operation while SELinux is in disabled state.
Sometimes we want to turn on SELinux enabled but can't because of huge downtime while autorelabeling. It could take more that a hour for > 1Tb of data.
If so, you may setup your SELinux environment in "offline" ( it's not official term, just my mind ), including list of modules, users, etc, then run offrestorecon and reboot your system without autorelabeling to discrease downtime to standart reboot downtime.
### How does it works?
I'd found that labels doesn't disappear when you switch SELinux from enabled to disabled. In other words it means that even if SELinux is disabled, some parts of it is accessible to get context, including default context matching ( matchpathcon ). So the tool just uses two libselinux calls: matchpathcon and setfilecon to restore default selinux context for some files.
### Is it production ready?
Yes. I wrote this piece of code as a part of my everyday duties to enable SELinux on high-loaded databases and fileshares.
### How to contribute?
Clone & send pull request, it's simple
### What about license?
This software is licensed unser GNU GPL ver 3.0
