%define version 0.9.6
%define release 1
%define name away

Summary: terminal locking
Name: away
Version: 0.9.6
Release: 1
Copyright: GPL
Group: Utilities/Console
Source: http://unbeatenpath.net/software/away/%{name}-%{version}.tar.gz
BuildRoot: /var/tmp/%{name}-%{version}-root
Url: http://unbeatenpath.net/software/away
Packager: Cameron Moore <cameron@unbeatenpath.net>

%description
%{name} is a terminal locking program with the additional ability
to check user-defined mailboxes for new mail.

%prep
rm -rf $RPM_BUILD_ROOT
%setup

%build
./configure --prefix=/usr
make CFLAGS="$RPM_OPT_FLAGS"

%install
make ROOT="$RPM_BUILD_ROOT" install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc AUTHORS BUGS COPYING ChangeLog NEWS README TODO
/etc/pam.d/away
/usr/bin/away
/usr/man/man1/away.1
/usr/man/man5/awayrc.5

%changelog
* Sat Jan 13 2001 Cameron Moore <cameron@unbeatenpath.net>
- created spec

