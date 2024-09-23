# Bucket-Storage (STL comaptible)

### The container is designed for scenarios with frequent insertions and deletions while maintaining stable element addresses in memory.

* Insertion, deletion, and iterator traversal are guaranteed to have constant time complexity **O(1)**.
* Pointers and iterators to stored elements remain valid throughout the object's lifetime, regardless of insertions or deletions of other elements.
* Didirectional iterators are supported.
