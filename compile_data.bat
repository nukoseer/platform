@echo off

pushd tools
pushd build

call .\build_sphere.exe > ..\..\src\sphere_data.inl
call .\shape_parser.exe --shapefile ..\ne_110m_admin_0_countries.shp --indices --points > ..\..\src\shape_data.inl

popd
popd
