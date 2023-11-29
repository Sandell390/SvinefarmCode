import 'package:cloud_firestore/cloud_firestore.dart';

class FoodContainer {
  late int precent = 0;
  late DateTime timestamp;

  FoodContainer({required this.precent});

  FoodContainer.fromJSON(Map<String, dynamic> json) {
    precent = json["precents"];
    timestamp = DateTime.fromMillisecondsSinceEpoch(
        (json["timestamp"] as Timestamp).millisecondsSinceEpoch);
  }
}
