Name:		cbridge
Version:	0.4
Release:	2.%{dist}.cern
Summary:	Program wich is reading and sending UDP data from one source to many destinations.
Packager:	Jiri Tyr

Group:		Productivity/Networking
License:	GPLv3
URL:		http://www.cern.ch
Source:		%{name}-%{version}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root

BuildRequires:	gcc
BuildRequires:	make
BuildRequires:	glibc-devel
BuildRequires:	perl(ExtUtils::Manifest)


%description
Cbridge is a C program which is reading UDP data from one source and sending the
same data to multiple destinations.


%prep
%setup -q -n %{name}-%{version}


%build
%{__make} %{?_smp_mflags}


%install
[ "%{buildroot}" != / ] && %{__rm} -rf "%{buildroot}"
%{__make} install DESTDIR=%{buildroot}


%clean
[ "%{buildroot}" != / ] && %{__rm} -rf "%{buildroot}"


%files
%defattr(-,root,root,-)
# For license text(s), see the perl package.
%doc Changes README
%{_bindir}/cbridge


%changelog
* Thu Oct 29 2009 Jiri Tyr <jiri.tyr at cern.ch>
- New version number.

* Wed May 6 2009 Jiri Tyr <jiri.tyr at cern.ch>
- First build.
