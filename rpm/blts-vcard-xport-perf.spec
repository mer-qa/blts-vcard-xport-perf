Name:       blts-vcard-xport-perf

Summary:    BLTS vcard storage performance test suite
Version:    0.0.1
Release:    0
Group:      Development/Testing
License:    GPLv2
URL:        https://github.com/mer-qa/blts-vcard-xport-perf
Source0:    %{name}-%{version}.tar.gz
Requires:   qtcontacts-sqlite-qt5
BuildRequires:  pkgconfig(bltscommon)
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Contacts)
BuildRequires:  pkgconfig(Qt5Versit)

%description
This package contains vcard storage performance tests


%prep
%setup -q -n %{name}-%{version}

%build
%qmake5 
%make_build

%install
%qmake5_install

%files
%defattr(-,root,root,-)
/opt/tests/blts-vcard-xport-perf/tests.xml
%attr(2755, root, privileged) /opt/tests/blts-vcard-xport-perf/blts-vcard-xport-perf
