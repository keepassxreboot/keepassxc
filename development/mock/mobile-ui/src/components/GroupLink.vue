<template>
  <q-item clickable tag="a" :to="title" :append="true">
    <q-item-section avatar>
      <q-icon name="folder" />
    </q-item-section>

    <q-item-section>
      <q-item-label>{{ title }}: {{ numEntries }} entries and {{ numChildren }} children</q-item-label>
    </q-item-section>
  </q-item>
</template>

<script>
import { defineComponent } from "vue";
import EntryView from "components/EntryView";

export default defineComponent({
  name: "GroupLink",
  components: {},
  computed: {
    numChildren() {
      return this.children.length
    },
    numEntries() {
      return this.children.length
    }
  },
  props: {
    title: {
      type: String,
      required: true,
    },
    children: {
      type: Array,
      required: true
    },
    entries: {
      type: Array,
      required: true
    }
  },
});
</script>
