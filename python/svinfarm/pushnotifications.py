import argparse
import json
from time import sleep
import asyncio
import aioschedule as schedule
from tkinter.tix import Tree
import requests
import google.auth.transport.requests
from Foodcontainer import FoodContainer
import firebase_admin
from firebase_admin import storage
from google.oauth2 import service_account
import firebase_admin
from firebase_admin import credentials
from firebase_admin import firestore

# Other initialization code...
# Create a new event loop and set it as the current loop
loop = asyncio.new_event_loop()
asyncio.set_event_loop(loop)

startedNotification = False

job:schedule.Job

cred = credentials.Certificate(
'SvinefarmGoogleServiceAccount.json')
firebase_admin.initialize_app(cred, options={"projectId": "svinefarm-5823e"})
db = firestore.client()

PROJECT_ID = '1023175142321'
BASE_URL = 'https://fcm.googleapis.com'
FCM_ENDPOINT = 'v1/projects/' + PROJECT_ID + '/messages:send'
FCM_URL = BASE_URL + '/' + FCM_ENDPOINT
SCOPES = ['https://www.googleapis.com/auth/firebase.messaging', "https://www.googleapis.com/auth/datastore"]

async def sendNotification():
    print("Checking for HasReacted")
    hasReacted = read_HasReacted()
    if hasReacted:
       cancel_notification()
    else:
      print("send notification")
      common_message = build_food_low_message()
      _send_fcm_message(common_message)
      


def set_critical_status(status: bool):
  doc_ref = db.collection('FoodAmount').document('Status')
  doc_ref.update({"Critical": status})

def set_has_reacted(state: bool):
  doc_ref = db.collection('FoodAmount').document('Status')
  doc_ref.update({"HasReacted": state})

def read_HasReacted() -> bool:
  doc_ref = db.collection('FoodAmount').document("Status")
  doc = doc_ref.get()
  hasReacted = doc.to_dict()["HasReacted"]
  print(hasReacted)
  return hasReacted

# Function to handle action for percent below 25
def action_normal():
    set_has_reacted(False)
    set_critical_status(False)

# Function to handle action for percent below 25
def action_below_25():
    set_critical_status(True)

# Function to handle action for percent below 15
def action_below_15():
   global startedNotification
   set_critical_status(True)
   if not startedNotification:
      start_notifaction()

def action_error():
  global startedNotification
  if not startedNotification:
    start_notifaction()

def start_notifaction():
   global startedNotification
   startedNotification = True
   global job
   job = schedule.every(20).seconds.do(sendNotification)
   print("Started sendNotification")

def cancel_notification():
    global startedNotification
    startedNotification = False
    global job
    if job in schedule.jobs:
        schedule.cancel_job(job)
        print("sendNotification cancelled.")
    
async def StoreNewReading():
  for i in range(len(schedule.jobs)):
     print(schedule.jobs[i])
  print("StoreNewReading is running...")
  # Get Food state
  foodState = get_food_status()

  print(f"Foodstate: {foodState}")
  match foodState:
    case 0:
      
      action_below_15()
    case 1:
      action_below_25()
    case 2:
      action_normal()
    case -1:
      action_error()

  add_new_sensor_reading()
  # Check if the food is so low that notifications need to be sent (15%)

def add_new_sensor_reading():
  foodStatus = get_food_status_distance()
  for i in range(len(foodStatus)):
    doc_ref = db.collection('FoodAmount').document('Sensors')
    doc = doc_ref.collection(f'Container{i}')
    doc.add({"precents":foodStatus[i].percent, 'timestamp': firestore.firestore.SERVER_TIMESTAMP})
  
  
def _get_access_token():
  credentials = service_account.Credentials.from_service_account_file(
    'SvinefarmGoogleServiceAccount.json', scopes=SCOPES)
  request = google.auth.transport.requests.Request()
  credentials.refresh(request)
  return credentials.token

def _send_fcm_message(fcm_message):
  headers = {
    'Authorization': 'Bearer ' + _get_access_token(),
    'Content-Type': 'application/json; UTF-8',
  }
  # [END use_access_token]
  resp = requests.post(FCM_URL, data=json.dumps(fcm_message), headers=headers)

  if resp.status_code == 200:
    print('Message sent to Firebase for delivery')
  else:
    print('Unable to send message to Firebase')
    print(resp.text)

def build_food_low_message():
  return {
    'message': {
      'topic': 'smart',
      'notification': {
        'title': 'Food is critical',
        'body': 'The silo is low on food'
      }
    }
  }

def get_food_status() -> int:
  response = requests.get('http://192.168.0.30/GetFoodState', timeout=10)
  print("FoodState | Status Code: ", response.status_code)
  foodState = -1

  if response.status_code == 200:
    obj = json.loads(response.text)
    foodState = obj["FoodState"];

  return foodState


def get_food_status_distance() -> list[FoodContainer]:
    response = requests.get('http://192.168.0.30/GetFoodPercent',timeout=10)

    print("FoodPercent | Status Code: ", response.status_code)

    if response.status_code == 200:
        objs = json.loads(response.text)
        print(objs)
        result = []
        for i in range(len(objs)):
            result.append(FoodContainer(objs[f"Container{i + 1}"]))

        print("FoodPercent | Good")
        return result

    print("FoodPercent | Not good")
    return []


async def run_scheduler():
    while True:
        await schedule.run_pending()
        await asyncio.sleep(1)

def main():
  # Schedule tasks
  schedule.every(30).seconds.do(StoreNewReading)

  # Run the scheduler in the new event loop
  loop.create_task(run_scheduler())
  loop.run_forever()  # This will run the ev  ent loop indefinitely

if __name__ == '__main__':
  main()
  #read_HasReacted()

