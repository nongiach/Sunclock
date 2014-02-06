%define name sunclock_jpeg_big_maps
%define version 1.0
%define release 1
%define prefix /usr

Summary: Big resolution maps package for Sunclock.
Summary(fr): Ensemble de cartes résolution moyenne pour Sunclock.
Name: %{name}
Version: %{version}
Release: %{release}
Serial: 0
Copyright: GPL
Group: Sciences/Astronomy
# Source: ftp://ftp.ac-grenoble.fr/ge/geosciences/%{name}-%{version}%{release}.tgz
Source: http://frmas.free.fr/files/%{name}-%{version}.tgz
URL: http://frmas.free.fr/li_1.htm
BuildRoot: %{_tmppath}/%{name}-buildroot
Prefix: %{_prefix}
# Requires: sunclock

%description
Sunclock displays a map of the Earth and shows which portion is illuminated
by the sun. It can commute between two states, the "clock window" and
the "map window". This file provides a lot of earth maps with different
definition for Sunclock.

%description -l fr
Sunclock affiche une carte de la Terre, et présente quelle portion est 
illuminée par le soleil. Il peut passer d'un aspect horloge carte 
miniature à une carte complete de la terre. Ce fichier fournit un ensemble
de cartes de la terre pour Sunclock.

%prep
rm -rf $RPM_BUILD_ROOT
%setup -q

%build


# %install
mkdir -p $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/big
install  jpeg/big/biomap_big.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/big/biomap_big.jpg
install  jpeg/big/earthlights.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/big/earthlights.jpg
install  jpeg/big/elevation_big.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/big/elevation_big.jpg
install  jpeg/big/photo_big1.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/big/photo_big1.jpg
install  jpeg/big/photo_big2.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/big/photo_big2.jpg
install  jpeg/big/population.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/big/population.jpg
install  jpeg/big/topography.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/big/topography.jpg
chmod 0644 $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/big/*

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,0644)
%{prefix}/share/sunclock/earthmaps/jpeg/big/biomap_big.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/big/earthlights.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/big/elevation_big.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/big/photo_big1.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/big/photo_big2.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/big/population.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/big/topography.jpg


%changelog
* Tue Oct 12 2001 Francois Massonneau <frmas@free.fr> 1.0-1
- First release
