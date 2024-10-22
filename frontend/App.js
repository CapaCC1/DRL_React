// src/App.js

import React from 'react';
import { SafeAreaView, ScrollView, StyleSheet, Text } from 'react-native';
import LEDControl from './components/LEDControl';

export default function App() {
  return (
    <SafeAreaView style={styles.container}>
      <ScrollView contentContainerStyle={styles.scrollView} scrollEnabled={false}>
        <Text style={styles.title}>LED Controller</Text>
        <LEDControl />
      </ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#424242',
    justifyContent: 'center',
    alignItems: 'center',
  },
  scrollView: {
    justifyContent: 'center',
    alignItems: 'center',
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    marginVertical: 20,
    color: '#919191'
  },
});
