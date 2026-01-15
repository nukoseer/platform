@echo off

pushd tools
pushd build

call .\build_sphere.exe > ..\..\src\sphere_data.inl
call .\shape_parser.exe --shapefile ..\ne_110m_admin_0_countries.shp --parts --indices --points
call .\dbf_parser.exe --dbf ..\ne_110m_admin_0_countries.dbf > ..\..\src\shape_meta_data.inl

popd
popd
