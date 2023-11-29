import 'package:firebase_core/firebase_core.dart';
import 'package:flutter/material.dart';
import 'package:svine_farm_app/screens/home.dart';
import 'package:firebase_messaging/firebase_messaging.dart';
import 'firebase_options.dart';
import 'package:flutter_local_notifications/flutter_local_notifications.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await Firebase.initializeApp(
    options: DefaultFirebaseOptions.currentPlatform,
  );
  await FirebaseMessaging.instance.setAutoInitEnabled(true);
  FirebaseMessaging.onBackgroundMessage(_firebaseMessagingBackgroundHandler);
  runApp(MyApp());
}

@pragma('vm:entry-point')
Future<void> _firebaseMessagingBackgroundHandler(RemoteMessage message) async {
  print("Handling a background message: ${message.data.toString()}");
}

class MyApp extends StatefulWidget {
  @override
  State<StatefulWidget> createState() => _MyApp();
}

class _MyApp extends State<MyApp> {
  final _scaffoldKey = GlobalKey<ScaffoldMessengerState>();
// It is assumed that all messages contain a data field with the key 'type'
  Future<void> setupInteractedMessage() async {
    // Get any messages which caused the application to open from
    // a terminated state.
    // You may set the permission requests to "provisional" which allows the user to choose what type
    // of notifications they would like to receive once the user receives a notification.

    NotificationSettings settings =
        await FirebaseMessaging.instance.requestPermission(
      alert: true,
      announcement: false,
      badge: true,
      carPlay: false,
      criticalAlert: false,
      provisional: false,
      sound: true,
    );

    print('User granted permission: ${settings.authorizationStatus}');

    final token = FirebaseMessaging.instance
        .getToken()
        .then((value) => print("Token: " + value.toString()));

    FirebaseMessaging.instance.onTokenRefresh.listen((fcmToken) {
      // TODO: If necessary send token to application server.
      print("New Token: " + fcmToken.toString());

      // Note: This callback is fired at each app startup and whenever a new
      // token is generated.
    }, onError: ((err) {
      // Error getting token.
      print("Error: " + err.toString());
    }), onDone: () {
      print("Done with token");
    });

    RemoteMessage? initialMessage =
        await FirebaseMessaging.instance.getInitialMessage();

    if (initialMessage != null) {
      _handleMessage(initialMessage);
    }

    // Also handle any interaction when the app is in the background via a
    // Stream listener
    FirebaseMessaging.onMessageOpenedApp.listen(_handleMessage);

    FirebaseMessaging.onMessage.listen((RemoteMessage message) {
      print('Got a message whilst in the foreground!');
      print('Message data: ${message.data}');

      if (message.notification != null) {
        showToast(message.notification!.body!);
      }
    });

    FirebaseMessaging.instance.subscribeToTopic("smart");
  }

  void _handleMessage(RemoteMessage message) {
    print("Message: " + message.toString());

    //Navigator.pushNamed(context, '/');
  }

  @override
  void initState() {
    super.initState();

    // Run code required to handle interacted messages in an async function
    // as initState() must not be async
    setupInteractedMessage();
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      routes: {
        '/test': (context) => const HomePage(),
      },
      debugShowCheckedModeBanner: false,
      title: 'Svinefarm Dashboard',
      theme: ThemeData(
        primarySwatch: Colors.blue,
      ),
      home: const HomePage(),
      scaffoldMessengerKey: _scaffoldKey,
    );
  }

  void showToast(String message) {
    final scaffold = _scaffoldKey.currentState;
    scaffold!.showSnackBar(
      SnackBar(
        content: Text(message),
        duration: const Duration(seconds: 5),
      ),
    );
  }
}
