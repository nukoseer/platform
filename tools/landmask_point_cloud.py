import sys
import netCDF4
import numpy as np
from math import cos, sin, pi

def polar_to_xyz(lon_deg, lat_deg):
    lon = lon_deg * pi / 180
    lat = lat_deg * pi / 180
    
    x = cos(lat) * cos(lon)
    y = sin(lat)
    z = cos(lat) * sin(lon)

    return x, y, z


def get_land_points(filename, min_lat=-84):
    with netCDF4.Dataset(filename) as dataset:
        lon = dataset.variables["lon"][:]
        lat = dataset.variables["lat"][:]
        lon2d, lat2d = np.meshgrid(lon, lat)

        sst_var = dataset.variables["sst"]
        sst0 = sst_var[0, :, :] if sst_var.ndim == 3 else sst_var[:, :]
        sst_ma = np.ma.array(sst0, mask=np.ma.getmaskarray(sst0))
        land = sst_ma.mask

        if min_lat is not None:
            land &= (lat2d > float(min_lat))

        # img = np.zeros_like(land, dtype=np.uint8)
        # img[land] = 255

        # img = np.flipud(img)

        # for i in range(len(img)):
        #     for j in range(len(img[i])):
        #         print(img[i][j], sep=", ", end=", ")

        ocean = ~land

        land_lons = lon2d[land].ravel()
        land_lats = lat2d[land].ravel()

        ocean_lons = lon2d[ocean].ravel()
        ocean_lats = lat2d[ocean].ravel()

        return [(land_lons, land_lats), (ocean_lons, ocean_lats)]


if __name__ == "__main__":
    if (sys.argv[1] != "--landmask-file") or (len(sys.argv) < 3):
        print("Usage: %s --landmask-file <path-to-netcdf-file>" % sys.argv[0])
        sys.exit(1)
    
    landmask_file = sys.argv[2]
    land_ocean = get_land_points(landmask_file)

    land_lons, land_lats = land_ocean[0]
    ocean_lons, ocean_lats = land_ocean[1]

    land_coordinates = []
    land_coordinates.extend([ polar_to_xyz(lon, lat) for lon, lat in zip(land_lons, land_lats)])

    ocean_coordinates = []
    ocean_coordinates.extend([ polar_to_xyz(lon, lat) for lon, lat in zip(ocean_lons, ocean_lats)])

    print("static vec3 global_landmask_vectors[] =\n{")
    for coordinate in land_coordinates:
        print("{ %ff, %ff, %ff }" % coordinate, sep=", ", end=", ")
    print("\n};\n")

    print("static vec3 global_ocean_vectors[] =\n{")
    for coordinate in ocean_coordinates:
        print("{ %ff, %ff, %ff }" % coordinate, sep=", ", end=", ")
    print("\n};\n")

