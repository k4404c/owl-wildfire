import pandas as pd
######################################## import sensor data and combine ########################################

jan_sep_sensor = pd.read_csv('/thai_fire_sensor_data/thai_sensor_data/thai_sensors_jan_sep_2024.csv',skiprows = 4)
jan_sep_sensor.head()


######################################## import fire data and combine ########################################

jan_sep_fires = pd.read_csv('/thai_fire_sensor_data/thai_fire_data/thai_fires_jan_sep_2024.csv')
jan_sep_fires.head()


######################################## clean sensor data by combining measurements into larger time intervals ########################################

import numpy as np

def resample_sensor_data(df, interval='15min'):
    """
    Resample sensor data to specified time intervals.

    Parameters:
    -----------
    df : pandas.DataFrame
        Input dataframe containing sensor measurements
    interval : str, default='15min'
        Pandas offset string for resampling interval

    Returns:
    --------
    pandas.DataFrame
        Resampled dataframe with aggregated measurements
    """
    # Make a copy to avoid modifying the original
    df = df.copy()

    # Convert timestamp column to datetime
    df['Datetime(UTC+0)'] = pd.to_datetime(df['Datetime(UTC+0)'])

    # Convert numeric columns to float, replacing any non-numeric values with NaN
    numeric_columns = [
        'GPS_Lat', 'GPS_Lng', 'GPS_Alt', 'Relative humidity',
        'PM1.0', 'PM10', 'PM2.5', 'Air pressure', 'Temperature',
        'Carbon monoxide(CO)', 'Carbon dioxide (CO2)'
        #dropped 'Formaldehyde (HCHO)', 'Nitrogen dioxide NO2' since they had NaN values
    ]

    for col in numeric_columns:
        df[col] = pd.to_numeric(df[col], errors='coerce')

    # Set datetime as index for resampling
    df.set_index('Datetime(UTC+0)', inplace=True)

    # Define aggregation methods for different column types
    agg_dict = {
        # Numeric sensors - take mean
        'GPS_Lat': 'mean',
        'GPS_Lng': 'mean',
        'GPS_Alt': 'mean',
        'Relative humidity': 'mean',
        'PM1.0': 'mean',
        'PM10': 'mean',
        'PM2.5': 'mean',
        'Air pressure': 'mean',
        'Temperature': 'mean',
        'Carbon monoxide(CO)': 'mean',
        'Carbon dioxide (CO2)': 'mean',

        # Keep the first values for identification columns
        'Node': 'first',
        'Timestamp': 'first'
    }

    # Resample and aggregate
    resampled = df.resample(interval).agg(agg_dict)

    # Reset index to make Datetime(UTC+0) a column again
    resampled.reset_index(inplace=True)

    # Round numeric columns to reasonable precision for speed and storage
    for col in numeric_columns:
        if col in resampled.columns:
            resampled[col] = resampled[col].round(6)

    # Forward fill missing values for sensor columns, but only within a reasonable window
    sensor_columns = ['PM1.0', 'PM10', 'PM2.5', 'Temperature',
                     'Relative humidity', 'Carbon monoxide(CO)',
                     'Carbon dioxide (CO2)', 'Air pressure']

    # Forward fill missing values for sensor columns, but only within a reasonable window
    resampled[sensor_columns] = resampled[sensor_columns].ffill(limit=2) # there should be no missing values, however.

    return resampled

def process_all_nodes(df, interval='15min'):
    """
    Process all nodes in the dataset (resample sensor data).

    Parameters:
    -----------
    df : pandas.DataFrame
        Input dataframe containing data for all nodes
    interval : str, default='15min'
        Resampling interval

    Returns:
    --------
    pandas.DataFrame
        Resampled data for all nodes
    """
    nodes = df['Node'].unique()
    resampled_dfs = []

    for node in nodes:
        node_data = df[df['Node'] == node].copy()
        resampled_node = resample_sensor_data(node_data, interval)
        resampled_dfs.append(resampled_node)

    # Combine all resampled data
    final_resampled = pd.concat(resampled_dfs, ignore_index=True)

    # Sort by datetime and node
    final_resampled.sort_values(['Datetime(UTC+0)', 'Node'], inplace=True)
    final_resampled.reset_index(drop=True, inplace=True)

    return final_resampled

# Resample sensor data on 15 minute intervals
jan_sep_sensor_resampled = process_all_nodes(jan_sep_sensor, interval='15min')

######################################## aggregate sensor data with fire data ########################################

from scipy.spatial import cKDTree
from datetime import timedelta
from typing import Tuple, List
import gc


def haversine_distance(lat1: np.ndarray, lon1: np.ndarray,
                      lat2: np.ndarray, lon2: np.ndarray) -> np.ndarray:
    """
    Calculate haversine distance between points (utilizing lat and lon).
    All inputs should be in degrees.
    """

    R = 6371  # Earth's radius in kilometers

    lat1, lon1, lat2, lon2 = map(np.radians, [lat1, lon1, lat2, lon2])
    dlat = lat2 - lat1
    dlon = lon2 - lon1

    a = np.sin(dlat/2.0)**2 + np.cos(lat1) * np.cos(lat2) * np.sin(dlon/2.0)**2
    c = 2 * np.arcsin(np.sqrt(a))
    return R * c


from scipy.spatial import cKDTree

def aggregate_sensor_fire_data_optimized(
    sensors_df: pd.DataFrame,
    fires_df: pd.DataFrame,
    distance_threshold: float = 20.0,
    time_window: timedelta = timedelta(hours=12)
) -> pd.DataFrame:
    """
    Optimized version using KD-Tree spatial indexing and vectorized operations.

    """

    print("Starting optimized data aggregation...")

    # Make copies to avoid modifying the original data
    sensors_df = sensors_df.copy()
    fires_df = fires_df.copy()

    # Vectorized datetime conversion
    sensors_df['Datetime'] = pd.to_datetime(sensors_df['Datetime(UTC+0)'])
    fires_df['Datetime'] = pd.to_datetime(
        fires_df['acq_date'] + ' ' + fires_df['acq_time'].astype(str).str.zfill(4).str[:2] + ':' +
        fires_df['acq_time'].astype(str).str.zfill(4).str[2:]
    )

    # Initialize columns with low cost dtypes
    sensors_df['fire_detected'] = False
    sensors_df['fire_distance'] = pd.Series(dtype='float32')
    sensors_df['fire_brightness'] = pd.Series(dtype='float32')
    sensors_df['fire_frp'] = pd.Series(dtype='float32')
    sensors_df['fire_confidence'] = pd.Series(dtype='float32')
    sensors_df['fires_nearby'] = pd.Series(dtype='int32')

    # Convert coordinates to radians
    def to_radians(degrees):
        return np.radians(degrees)

    # Process in temporal chunks
    chunk_size = timedelta(days=1)
    current_time = sensors_df['Datetime'].min()
    end_time = sensors_df['Datetime'].max()

    while current_time < end_time:
        chunk_end = current_time + chunk_size
        print(f"Processing chunk: {current_time}")

        # Get current chunk of sensors
        sensor_mask = (sensors_df['Datetime'] >= current_time) & (sensors_df['Datetime'] < chunk_end)
        current_sensors = sensors_df[sensor_mask].copy()

        if len(current_sensors) == 0:
            current_time = chunk_end
            continue

        # Filter out invalid sensor coordinates
        valid_sensor_mask = (
            current_sensors['GPS_Lat'].notna() &
            current_sensors['GPS_Lng'].notna() &
            np.isfinite(current_sensors['GPS_Lat']) &
            np.isfinite(current_sensors['GPS_Lng'])
        )
        current_sensors = current_sensors[valid_sensor_mask]

        if len(current_sensors) == 0:
            print(f"No valid sensor coordinates in chunk {current_time}")
            current_time = chunk_end
            continue

        # Get relevant fires with extended time window
        fire_mask = (
            (fires_df['Datetime'] >= current_time - time_window) &
            (fires_df['Datetime'] <= chunk_end + time_window)
        )
        current_fires = fires_df[fire_mask].copy()

        if len(current_fires) == 0:
            current_time = chunk_end
            continue

        # Filter out invalid fire coordinates
        valid_fire_mask = (
            current_fires['latitude'].notna() &
            current_fires['longitude'].notna() &
            np.isfinite(current_fires['latitude']) &
            np.isfinite(current_fires['longitude'])
        )
        current_fires = current_fires[valid_fire_mask]

        if len(current_fires) == 0:
            print(f"No valid fire coordinates in chunk {current_time}")
            current_time = chunk_end
            continue

        # Build KD-Tree for spatial searching
        fires_coords = np.column_stack((
            to_radians(current_fires['latitude'].values),
            to_radians(current_fires['longitude'].values)
        ))
        tree = cKDTree(fires_coords)

        # Convert sensor coordinates
        sensors_coords = np.column_stack((
            to_radians(current_sensors['GPS_Lat'].values),
            to_radians(current_sensors['GPS_Lng'].values)
        ))

        # Find all fires within distance threshold
        search_radius = distance_threshold / 6371.0  # Earth's radius in km
        indices = tree.query_ball_point(sensors_coords, search_radius)

        # Process results
        for sensor_idx, fire_indices in enumerate(indices):
            if not fire_indices:  # Skip if no fires found
                continue

            sensor = current_sensors.iloc[sensor_idx]
            nearby_fires = current_fires.iloc[fire_indices]

            # Time filter
            time_valid = (
                (nearby_fires['Datetime'] >= sensor['Datetime'] - time_window) &
                (nearby_fires['Datetime'] <= sensor['Datetime'] + time_window)
            )

            if time_valid.any():
                valid_fires = nearby_fires[time_valid]

                # Calculate exact distances for valid fires
                valid_coords = np.column_stack((
                    to_radians(valid_fires['latitude'].values),
                    to_radians(valid_fires['longitude'].values)
                ))
                sensor_coord = sensors_coords[sensor_idx]

                # Haversine formula vectorized
                dlat = valid_coords[:, 0] - sensor_coord[0]
                dlon = valid_coords[:, 1] - sensor_coord[1]
                a = np.sin(dlat/2)**2 + np.cos(sensor_coord[0]) * np.cos(valid_coords[:, 0]) * np.sin(dlon/2)**2
                distances = 2 * 6371.0 * np.arcsin(np.sqrt(a))  # Distance in km

                closest_idx = distances.argmin()
                closest_distance = distances[closest_idx]
                closest_fire = valid_fires.iloc[closest_idx]

                # Update sensor data with explicit type casting
                idx = current_sensors.index[sensor_idx]
                sensors_df.loc[idx, 'fire_detected'] = True
                sensors_df.loc[idx, 'fire_distance'] = np.float32(closest_distance)
                sensors_df.loc[idx, 'fire_brightness'] = np.float32(closest_fire['brightness'])
                sensors_df.loc[idx, 'fire_frp'] = np.float32(closest_fire['frp'])
                
                # Handle non-numeric confidence values
                if isinstance(closest_fire['confidence'], (int, float)):
                    sensors_df.loc[idx, 'fire_confidence'] = np.float32(closest_fire['confidence'])
                else:
                    sensors_df.loc[idx, 'fire_confidence'] = np.float32(0)  # Default value for non-numeric confidence
                    
                sensors_df.loc[idx, 'fires_nearby'] = np.int32(len(valid_fires))

        current_time = chunk_end
        gc.collect() #garbage collection to free up memory

    print("Processing complete!")
    print(f"Total readings: {len(sensors_df)}")
    print(f"Readings with fires nearby: {sensors_df['fire_detected'].sum()}")

    return sensors_df



combined_data = aggregate_sensor_fire_data_optimized(
    jan_sep_sensor_resampled,
    jan_sep_fires,
    distance_threshold=15.0, #km
    time_window=timedelta(hours=12) #+/-
)


######################################## save aggregated data to csv ########################################

combined_data.to_csv('thai_fire_sensor_data.csv', index=False)


#############################################################################################################
