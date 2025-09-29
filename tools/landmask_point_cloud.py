import netCDF4
import numpy as np
from math import cos, sin, pi

#LANDMASK_FILE = "./sst.oisst.mon.ltm.1991-2020.nc"
LANDMASK_FILE = "./sst.ltm.1981-2010.nc"

def polar_to_xyz(lon_deg, lat_deg):
    lon = lon_deg * pi / 180
    lat = lat_deg * pi / 180
    
    x = cos(lat) * cos(lon)
    y = sin(lat)
    z = cos(lat) * sin(lon)

    return x, y, z


def get_land_points(filename=LANDMASK_FILE, min_lat=-84):
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

        land = ~land

        lons = lon2d[land].ravel()
        lats = lat2d[land].ravel()

        return (lons, lats)

if __name__ == "__main__":
    lons, lats = get_land_points()

    coordinates = []

    coordinates.extend([ polar_to_xyz(lon, lat) for lon, lat in zip(lons, lats)])

    for coordinate in coordinates:
        print("{ %ff, %ff, %ff }" % coordinate, sep=", ", end=", ")
