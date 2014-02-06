%define name sunclock
%define version 3.54
%define release 1
%define prefix /usr

Summary: The sophisticated clock for the X Window system.
Summary(fr): Une représentation de l'illumination terrestre.
Name: %{name}
Version: %{version}
Release: %{release}
Serial: 0
Copyright: GPL
Group: Sciences/Astronomy
# Source: ftp://ftp.ac-grenoble.fr/ge/geosciences/%{name}-%{version}%{release}.tgz
Source: ftp://ftp.ac-grenoble.fr/ge/geosciences/%{name}-%{version}.tgz
URL: http://freshmeat.net/projects/sunclock/
BuildRoot: %{_tmppath}/%{name}-buildroot
Prefix: %{_prefix}
Requires: libc.so.6(GLIBC_2.0)

%description
Sunclock displays a map of the Earth and shows which portion is illuminated
by the sun. It can commute between two states, the "clock window" and
the "map window". The clock window displays a small map of the Earth
and therefore occupies little space on the screen, while the "map window" 
displays a large map and offers more advanced functions. 

%description -l fr
Sunclock affiche une carte de la Terre, et présente quelle portion est 
illuminée par le soleil. Il peut passer d'un aspect horloge carte 
miniature à une carte complete de la terre. Plusieurs fonctionnalités sont
implémentées, notamment la possibilité de changer de cartes à volonté,
un affichage ou non de la zone éclairée, du soleil, etc...

%prep
rm -rf $RPM_BUILD_ROOT
%setup -q

%build
xmkmf
make CDEBUGFLAGS="$RPM_OPT_FLAGS" CXXDEBUGFLAGS="$RPM_OPT_FLAGS"

# %install
make install DESTDIR=$RPM_BUILD_ROOT%{prefix}
make install.man DESTDIR=$RPM_BUILD_ROOT%{prefix}
if [ -f $RPM_BUILD_ROOT%{prefix}/X11R6/man/man1/sunclock.1x ]; then
find $RPM_BUILD_ROOT%{prefix}/X11R6/man/man1/sunclock.1x -type f -exec bzip2 -9f {} \;
fi
# ajouté les trois lignes suivantes le 26/06/2001
mkdir -p $RPM_BUILD_ROOT/usr/share/icons
	# install wm_icons/sunclock.xpm -m 644 $RPM_BUILD_ROOT/usr/share/icons/sunclock.xpm
install wm_icons/sunclock2.xpm -m 644 $RPM_BUILD_ROOT/usr/share/icons/sunclock2.xpm
# ajouté les six lignes suivantes le 30/08/2001
mkdir -p $RPM_BUILD_ROOT/usr/share/applnk-mdk/Favorites/Sciences
install /home/fm/RPM/SPECS/Sunclock.kdelnk.file -m 644 $RPM_BUILD_ROOT/usr/share/applnk-mdk/Favorites/Sciences/Sunclock.desktop
install /home/fm/RPM/SPECS/.directory.sunclock.file -m 644 $RPM_BUILD_ROOT/usr/share/applnk-mdk/Favorites/.directory
mkdir -p $RPM_BUILD_ROOT/usr/share/applnk/Favorites/Sciences
install /home/fm/RPM/SPECS/Sunclock.kdelnk.file -m 644 $RPM_BUILD_ROOT/usr/share/applnk/Favorites/Sciences/Sunclock.kdelnk
install /home/fm/RPM/SPECS/.directory.sunclock.file -m 644 $RPM_BUILD_ROOT/usr/share/applnk/Favorites/.directory

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,0755)
%doc CHANGES coordinates.txt COPYING INSTALL README TODO VMF.txt WARNING
%{prefix}/share/sunclock/Sunclockrc
%{prefix}/share/sunclock/earthmaps/vmf/*
%{prefix}/share/sunclock/i18n/*
%{prefix}/X11R6/bin/sunclock
%{prefix}/X11R6/man/man1/sunclock.1*
# ajouté les deux lignes suivantes le 26/06/2001
	# %{prefix}/share/icons/sunclock.xpm
%{prefix}/share/icons/sunclock2.xpm
# ajouté les quatre lignes suivantes le 30/08/2001
%{prefix}/share/applnk-mdk/Favorites/Sciences/Sunclock.desktop
%{prefix}/share/applnk/Favorites/Sciences/Sunclock.kdelnk
%{prefix}/share/applnk-mdk/Favorites/.directory
%{prefix}/share/applnk/Favorites/.directory


%changelog
* Tue Aug 30 2001 Francois Massonneau <frmas@free.fr>
- Add files needed for K Menu entries
* Tue Aug 14 2001 Francois Massonneau <frmas@free.fr>
- Change the spec file to match the 3.5x serie
* Tue Jun 26 2001 Francois Massonneau <frmas@free.fr>
- Add the Sunclock icon during the install
* Tue Mar 06 2001 Francois Massonneau <frmas@free.fr>
- First spec file
