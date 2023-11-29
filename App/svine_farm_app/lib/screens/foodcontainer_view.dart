import 'dart:async';
import 'dart:io';
import 'dart:typed_data';
import 'package:intl/intl.dart';
import 'dart:ui' as ui;
import 'dart:math' as math;

import 'package:flutter/material.dart';
import 'package:svine_farm_app/models/foodContainers.dart';
import 'package:cloud_firestore/cloud_firestore.dart';
import 'package:widget_mask/widget_mask.dart';

class FoodcontainerView extends StatefulWidget {
  const FoodcontainerView({super.key});

  @override
  State<FoodcontainerView> createState() => FoodcontainerViewsState();
}

class FoodcontainerViewsState extends State<FoodcontainerView> {
  @override
  void initState() {
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return FutureBuilder<List<Stream<QuerySnapshot<Map<String, dynamic>>>>>(
        future: GetContainerStreams(),
        builder: ((context, snapshotFuture) {
          if (!snapshotFuture.hasData || snapshotFuture.data == null) {
            return const CircularProgressIndicator();
          }
          return GridView.builder(
              itemCount: snapshotFuture.data!.length,
              gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                childAspectRatio: 0.5,
                crossAxisCount: 2,
              ),
              itemBuilder: (context, index) {
                return StreamBuilder(
                    stream: snapshotFuture.data![index],
                    builder: (context, snapshotStrem) {
                      if (!snapshotStrem.hasData ||
                          snapshotStrem.data == null) {
                        return const CircularProgressIndicator();
                      }
                      var containers = <FoodContainer>[];
                      for (var i = 0;
                          i < snapshotStrem.data!.docs.length;
                          i++) {
                        containers.add(FoodContainer.fromJSON(
                            snapshotStrem.data!.docs[i].data()));
                        containers.sort((a, b) {
                          return -a.timestamp.compareTo(b.timestamp);
                        });
                      }
                      return Card(
                          child: Column(
                        crossAxisAlignment: CrossAxisAlignment.center,
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          RotatedBox(
                              quarterTurns: 3,
                              child: SizedBox(
                                width: 100,
                                child: WidgetMask(
                                  blendMode: BlendMode.lighten,
                                  mask: Transform.scale(
                                    scaleX: 0.85,
                                    child: Transform.translate(
                                      offset: const Offset(10, 0),
                                      child: LinearProgressIndicator(
                                        color: Colors.green,
                                        backgroundColor: Colors.transparent,
                                        borderRadius: const BorderRadius.all(
                                            Radius.circular(1)),
                                        minHeight: 120,
                                        value: containers[0].precent / 100,
                                      ),
                                    ),
                                  ),
                                  child: const Image(
                                      image: AssetImage("assets/silo.png")),
                                ),
                              )),
                          Text("${containers[0].precent}%"),
                          Container(
                              padding:
                                  const EdgeInsets.symmetric(horizontal: 25),
                              child: Text(DateFormat('yyyy-MM-dd')
                                  .format(containers[0].timestamp))),
                          Container(
                              padding:
                                  const EdgeInsets.symmetric(horizontal: 25),
                              child: Text(DateFormat('kk:mm')
                                  .format(containers[0].timestamp)))
                        ],
                      ));
                    });
              });
        }));
  }

  Future<List<Stream<QuerySnapshot<Map<String, dynamic>>>>>
      GetContainerStreams() async {
    final db = FirebaseFirestore.instance;

    List<Stream<QuerySnapshot<Map<String, dynamic>>>> result =
        <Stream<QuerySnapshot<Map<String, dynamic>>>>[];

    final sensors = db.collection("FoodAmount").doc("Sensors");
    await sensors.get().then(
      (DocumentSnapshot doc) async {
        final data = doc.data() as Map<String, dynamic>;

        int count = int.parse(data["total"]);

        for (int i = 0; i < count; i++) {
          final collection = sensors.collection("Container$i");
          String name = collection.id;
          print("Getting stream name $name");

          result.add(collection.snapshots(includeMetadataChanges: true));
        }
      },
      onError: (e) => print("Error getting document: $e"),
    );

    return result;
  }
}
