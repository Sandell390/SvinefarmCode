class FoodContainer:
    def __init__(self, json_data):
        percent = json_data.get("precent", 0)  # Default to 0 if "precent" key is not found
        self.percent = percent