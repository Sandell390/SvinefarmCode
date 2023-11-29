import 'package:cloud_firestore/cloud_firestore.dart';
import 'package:flutter/material.dart';
import 'package:svine_farm_app/screens/foodcontainer_view.dart';

class HomePage extends StatelessWidget {
  const HomePage({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Row(
          crossAxisAlignment: CrossAxisAlignment.center,
          children: [
            const Text("Text1"),
            Expanded(
              child: Container(),
            ),
            StreamBuilder(
                stream: FirebaseFirestore.instance
                    .collection("FoodAmount")
                    .doc("Status")
                    .snapshots(includeMetadataChanges: true),
                builder: (context, snapshot) {
                  if (!snapshot.hasData || snapshot.data == null) {
                    return const CircularProgressIndicator();
                  }

                  bool critical = snapshot.data!.get("Critical");
                  bool hasReacted = snapshot.data!.get("HasReacted");

                  if (!critical) {
                    return const Icon(
                      size: 50,
                      Icons.warning_amber,
                      color: Color.fromARGB(20, 0, 0, 0),
                    );
                  } else {
                    return Row(
                      children: [
                        !hasReacted
                            ? OutlinedButton(
                                onPressed: () {
                                  FirebaseFirestore.instance
                                      .collection("FoodAmount")
                                      .doc("Status")
                                      .update({"HasReacted": true});
                                },
                                child: const Text("React"))
                            : const OutlinedButton(
                                onPressed: null, child: Text("Has reacted")),
                        const Icon(
                          size: 50,
                          Icons.warning_amber,
                          color: Color.fromARGB(255, 255, 0, 0),
                        ),
                      ],
                    );
                  }
                }),
          ],
        ),
      ),
      body: const Center(
        child: FoodcontainerView(),
      ),
    );
  }
}
