<template>
  <q-list>
    <GroupLink v-for="group in children" v-bind="group"></GroupLink>
    <EntryView v-for="entry in entries" v-bind="entry"></EntryView>
  </q-list>
</template>

<script>
import { defineComponent } from "vue";
import EntryView from "components/EntryView";
import GroupLink from "components/GroupLink";

export default defineComponent({
  name: "GroupView",
  components: {GroupLink, EntryView},
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
