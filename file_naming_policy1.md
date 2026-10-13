# Per-Resource File Naming Policy Plan

## Goal

iRODS currently uses a server-wide vault path policy, normally configured via
`acSetVaultPathPolicy`, to decide whether physical paths should follow the
logical namespace or use a random naming scheme. This should become a
per-resource setting so different storage resources can make independent
decisions.

The new policy is stored in each resource's context string using the key
`file_naming_policy`.

## Decisions

- This is a hard cutover from `acSetVaultPathPolicy` for physical path naming.
- If `file_naming_policy` is absent, use the shipped default behavior:
  `consistent`.
- Initial supported policy values are `consistent` and `random`.
- Policy values are case-sensitive to make debugging easier.
- Unknown policy values must log a warning and behave as `consistent`.
- Duplicate context keys must log a warning explaining the values found and the
  value being used.
- Duplicate-key resolution should follow current effective behavior: the last
  value wins.
- Random naming options should keep the existing key names:
  `random_scheme_style` and `random_scheme_suffix_length`.
- Invalid random style or suffix values should log warnings and use defaults.
- The policy must be read from the leaf resource where the replica is stored,
  not from coordinating resources such as replication or passthru resources.
- Upgrade/migration logic is intentionally left for a later change.

## Rationale

Logical paths and physical storage paths are separated in the catalog, but the
default consistent layout keeps them in sync. Logical moves can therefore result
in physical renames through the resource plugin. That is expensive for recursive
collection moves.

Random naming avoids the need to keep physical paths in sync with logical paths.
Because storage resources can have different operational requirements, this
choice should be made per resource instead of per server.

The runtime implementation should be separated from upgrade behavior. Upgrade
may later inspect the old `acSetVaultPathPolicy` intent and set
`file_naming_policy` on each resource. That work touches migration and persisted
administrator intent, so it should be handled independently from the runtime
cutover.

## Runtime Semantics

### `consistent`

- New writes use the existing consistent/graft-style physical path layout.
- Logical data object moves physically rename replicas to keep physical paths in
  sync.
- Recursive collection moves continue to call physical rename for replicas on
  consistent-policy leaf resources.

### `random`

- New writes use the existing random physical path layout.
- `random_scheme_style` and `random_scheme_suffix_length` affect only new writes
  into that resource.
- Logical data object moves do not physically rename replicas.
- Recursive collection moves skip physical rename for replicas on random-policy
  leaf resources.

### Existing Replicas

Changing a resource's `file_naming_policy` does not rewrite existing physical
paths. The setting affects future writes and future move/sync behavior only.

For example, if an administrator changes a resource from `consistent` to
`random`, existing consistent-looking physical paths remain as-is and will no
longer be physically renamed on later logical moves.

## Implementation Notes

The main implementation area is `server/core/src/physPath.cpp`.

Relevant existing behavior:

- `getFilePathName()` currently calls `getVaultPathPolicy()` and chooses between
  `setPathForGraftPathScheme()` and `setPathForRandomScheme()`.
- `syncDataObjPhyPathS()` currently calls `getVaultPathPolicy()` and returns
  early when the scheme is not `GRAFT_PATH_S`.
- `syncCollPhyPath()` calls `syncDataObjPhyPathS()` for each replica found under
  the moved collection, so the leaf-resource decision in `syncDataObjPhyPathS()`
  is the important recursive-move optimization point.
- `physPath.cpp` currently stores random scheme style and suffix length in
  mutable process-global variables. Those should be removed.

Suggested implementation steps:

1. Add constants and enum-like helpers in or near
   `server/core/include/irods/vault_path_policy.hpp`.

   Suggested constants:

   - `file_naming_policy`
   - `file_naming_policy_consistent`
   - `file_naming_policy_random`
   - existing `random_scheme_style`
   - existing `random_scheme_suffix_length`

2. Add a small internal policy model in `physPath.cpp`.

   Example shape:

   ```cpp
   enum class file_naming_policy
   {
       consistent,
       random
   };

   struct file_naming_policy_config
   {
       file_naming_policy policy = file_naming_policy::consistent;
       int random_scheme_style = irods::vault_path_policy::random_scheme_config_default_style;
       int random_scheme_suffix_length = irods::vault_path_policy::random_scheme_config_default_suffix_length;
   };
   ```

3. Add defensive resource-context parsing.

   Requirements:

   - Resolve context from the leaf resource, preferably via
     `irods::get_resource_property<std::string>(rescId, irods::RESOURCE_CONTEXT,
     context)`.
   - Tolerate malformed/non-`key=value` tokens because existing context strings
     can contain legacy/freeform values.
   - Detect duplicate keys for the policy-related keys and log a warning showing
     all values and the value used.
   - Use last value wins.
   - Do not make malformed unrelated tokens fatal.
   - Log warnings for invalid policy, invalid style, invalid suffix length, and
     malformed numeric values.

4. Replace `getVaultPathPolicy()` usage in `getFilePathName()`.

   Current logic should become policy driven:

   - `consistent` calls existing `setPathForGraftPathScheme()` with the current
     shipped defaults equivalent to `msiSetGraftPathScheme("no", "1")`.
   - `random` calls `setPathForRandomScheme()` using the per-resource style and
     suffix length.

   The existing `getVaultPathPolicy()` function can remain for ABI/source
   compatibility, but it should no longer control physical path generation.

5. Replace `getVaultPathPolicy()` usage in `syncDataObjPhyPathS()`.

   The sync decision should become:

   - If policy is `consistent`, continue current sync behavior.
   - If policy is anything else, return early before `getFilePathName()` and
     before any `rsFileRename()` call.

6. Remove mutable process-global random naming state.

   `random_scheme_style` and `random_scheme_suffix_length` should be passed into
   random path generation explicitly, most likely by changing
   `setPathForRandomScheme()` or by adding a small wrapper.

7. Preserve old symbols initially.

   Leave `msiSetRandomScheme`, `msiSetGraftPathScheme`, random-scheme
   microservices, and `getVaultPathPolicy()` in place for now. Existing rules may
   still reference them, but they no longer control physical path naming after
   this cutover.

## Extensibility

The design should make future policies straightforward. One likely future value
is `reversed_dataid`, intended to produce physical paths that are more
recoverable for a storage administrator than fully random paths. This policy may
also provide a clean migration target for the S3 resource plugin's existing
`ARCHIVE_NAMING_POLICY=decoupled` behavior if implemented carefully.

The S3 resource plugin currently treats `ARCHIVE_NAMING_POLICY=decoupled` as a
data-ID-based naming policy. New S3 objects are stored using a key like
`/<bucket>/<reversed_data_id>/<object_name>`, and logical moves do not physically
rename the S3 object. That is not equivalent to `file_naming_policy=random`,
because `random` would change the object-key layout and reduce recoverability
for storage administrators.

A future `file_naming_policy=reversed_dataid` should be considered the closest
semantic replacement for `ARCHIVE_NAMING_POLICY=decoupled`:

- New writes use a physical naming scheme based on the reversed data ID.
- Logical moves skip physical rename because the physical name is no longer
  expected to track the logical path.
- Existing S3 `decoupled` resources can be migrated without changing their
  naming family.
- S3-specific `ARCHIVE_NAMING_POLICY` can then be deprecated and eventually
  removed.

Adding future policies should require only:

- Add enum value.
- Add string parser branch.
- Add path-generation branch.
- Add move/sync decision branch.
- Add tests and documentation.

Do not encode random styles as `file_naming_policy` values for now. Keep
`file_naming_policy` focused on the naming family and use separate context keys
for family-specific options.

Do not migrate S3 `ARCHIVE_NAMING_POLICY=decoupled` directly to
`file_naming_policy=random`. That would preserve the no-rename behavior, but it
would not preserve S3's data-ID-based object-key layout.

## Tests

Update or add Python tests around resource context behavior.

Required coverage:

1. Missing `file_naming_policy` produces the current consistent layout.
2. `file_naming_policy=consistent` produces consistent layout and physical
   rename on logical move.
3. `file_naming_policy=random` produces random layout and no physical rename on
   logical move.
4. Recursive collection move with mixed resources only physically renames
   replicas on consistent-policy leaf resources.
5. Invalid `file_naming_policy` logs a warning and behaves as `consistent`.
6. Duplicate `file_naming_policy` logs a warning identifying observed values and
   the value used.
7. Invalid `random_scheme_style` logs a warning and uses the default.
8. Invalid `random_scheme_suffix_length` logs a warning and uses the default.
9. Random style and suffix context keys affect only new writes into that
   resource.

Existing tests around `acSetVaultPathPolicy` and random scheme customization in
`scripts/irods/test/test_all_rules.py` will need to be removed or rewritten
because the policy is no longer authoritative for physical path naming.

Tests that modify resource context should account for resource manager caching.
Use a new connection or server reload after modifying resource context, matching
existing test patterns.

## Documentation

Documentation should cover:

- `file_naming_policy=consistent|random`.
- The default value is `consistent`.
- Values are case-sensitive.
- The setting is leaf-resource scoped.
- Changing the setting does not rewrite existing physical paths.
- The setting affects future writes and future move/sync behavior.
- `random_scheme_style` and `random_scheme_suffix_length` are resource context
  keys and only apply to random naming.
- `msiSetRandomScheme`, `msiSetGraftPathScheme`, and related random-scheme
  microservices no longer control physical path naming after this cutover.
