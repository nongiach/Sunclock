%define name sunclock_maps_package
%define version 1.0
%define release 1
%define prefix /usr

Summary: Earth maps package for Sunclock, the sophisticated clock.
Summary(fr): Ensemble de cartes pour le programme Sunclock.
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
mkdir -p $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/large
mkdir -p $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg
install  altitude.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/altitude.jpg
install  biomap.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/biomap.jpg
install  caida.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/caida.jpg
install  crustalplates.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/crustalplates.jpg
install  depthmap.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/depthmap.jpg
install  elevation.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/elevation.jpg
install  photo1.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/photo1.jpg
install  photo2.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/photo2.jpg
install  rainmap.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/rainmap.jpg
install  temperature.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/temperature.jpg
install  biomap_big.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/large/biomap_big.jpg
install  earthlights.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/large/earthlights.jpg
install  photo_big.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/large/photo_big.jpg
install  photo_medium.jpg $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/large/photo_medium.jpg
chmod 0644 $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/jpeg/*
chmod 0644 $RPM_BUILD_ROOT/usr/share/sunclock/earthmaps/large/*

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,0644)
%{prefix}/share/sunclock/earthmaps/jpeg/altitude.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/biomap.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/caida.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/crustalplates.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/depthmap.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/elevation.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/photo1.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/photo2.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/rainmap.jpg
%{prefix}/share/sunclock/earthmaps/jpeg/temperature.jpg
%{prefix}/share/sunclock/earthmaps/large/biomap_big.jpg
%{prefix}/share/sunclock/earthmaps/large/earthlights.jpg
%{prefix}/share/sunclock/earthmaps/large/photo_big.jpg
%{prefix}/share/sunclock/earthmaps/large/photo_medium.jpg


%changelog
* Tue Jun 12 2001 Francois Massonneau <frmas@free.fr>
- First release
