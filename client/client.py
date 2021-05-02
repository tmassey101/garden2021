import requests
from random import random
import time

# API address
link = "https://garden2021.herokuapp.com/insert?t="


while True:

    # Get sensor reading
    reading = 5 + (10.0 * random() )    # Placeholder

    # Combine link address with reading
    url = link+str(reading)

    try:
        response = requests.get(url)
        # If the response was successful, no Exception will be raised
        response.raise_for_status()
    except Exception as err:
        print('Other error occurred: %s', err)  
    else:
        print('Success! Posted reading = %f to %s' % (reading, url) )

    print('Waiting 60 secs...')
    time.sleep(60)

