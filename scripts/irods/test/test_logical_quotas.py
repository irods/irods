import os
import sys
import unittest

from . import session
from .. import lib
from .. import paths
from .. import test
from .resource_suite import ResourceBase
from ..configuration import IrodsConfig
from ..controller import IrodsController


class Test_Logical_Quotas(
    session.make_sessions_mixin([("otherrods", "rods")], []), unittest.TestCase
):

    def setUp(self):
        super(Test_Logical_Quotas, self).setUp()

        self.admin = self.admin_sessions[0]

        self.quota_user = session.mkuser_and_return_session(
            "rodsuser", "lquotauser", "lquotapass", lib.get_hostname()
        )
        self.llq_output_template = (
            "Collection name: %s\n"
            "Maximum bytes: %s\n"
            "Maximum objects: %s\n"
            "Bytes over: %s\n"
            "Objects over: %s"
        )

    def tearDown(self):
        self.quota_user.__exit__()
        self.admin.assert_icommand(["iadmin", "rmuser", "lquotauser"])
        super(Test_Logical_Quotas, self).tearDown()

    def test_logical_quota_set_nonexistent(self):
        self.admin.assert_icommand(
            [
                "iadmin",
                "set_logical_quota",
                f"{self.quota_user.session_collection}/blah",
                "bytes",
                "10000",
            ],
            "STDERR_SINGLELINE",
            "-816000 CAT_INVALID_ARGUMENT",
        )

    def test_logical_quota_list_nonexistent(self):
        self.admin.assert_icommand(
            [
                "iadmin",
                "list_logical_quotas",
                f"{self.quota_user.session_collection}/blah",
            ],
            "STDERR",
            ["CAT_UNKNOWN_COLLECTION", "-814000"],
        )

    def test_logical_quota_set_bad_values(self):
        try:
            # Nonexistent keyword
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    f"{self.quota_user.session_collection}",
                    "potatoes",
                    "10000",
                ],
                "STDERR_SINGLELINE",
                "-130000 SYS_INVALID_INPUT_PARAM",
            )

            # Negative values
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    f"{self.quota_user.session_collection}",
                    "bytes",
                    "--",
                    "-10000",
                ],
                "STDERR_SINGLELINE",
                "-349000 USER_INPUT_FORMAT_ERR",
            )

            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "objects",
                    "--",
                    "-10000",
                ],
                "STDERR_SINGLELINE",
                "-349000 USER_INPUT_FORMAT_ERR",
            )
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "0",
                    "--",
                    "-10000",
                ],
                "STDERR_SINGLELINE",
                "-349000 USER_INPUT_FORMAT_ERR",
            )
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "--",
                    "-10000",
                    "10000",
                ],
                "STDERR_SINGLELINE",
                "-349000 USER_INPUT_FORMAT_ERR",
            )
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "--",
                    "-10000",
                    "-10000",
                ],
                "STDERR_SINGLELINE",
                "-349000 USER_INPUT_FORMAT_ERR",
            )

            # Invalid final parameter
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "bytes",
                    "tomatoes",
                ],
                "STDERR_SINGLELINE",
                "-130000 SYS_INVALID_INPUT_PARAM",
            )

            # Numeric overflow
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "bytes",
                    "218128128218374562312312412412",
                ],
                "STDERR_SINGLELINE",
                "-130000 SYS_INVALID_INPUT_PARAM",
            )

        finally:
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "0",
                    "0",
                ]
            )

    def test_logical_quota_set(self):
        try:
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "bytes",
                    "10000",
                ]
            )
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        "10000",
                        "<unset>",
                        "-10000",
                        "<unenforced>",
                    )
                )
                in out
            )
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "objects",
                    "10000",
                ]
            )
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )

            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        "10000",
                        "10000",
                        "-10000",
                        "0",
                    )
                )
                in out
            )

        finally:
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "0",
                    "0",
                ]
            )

    def test_logical_quota_enforcement(self):
        file_name = "test_logical_quota_enforcement"
        file_path = os.path.join(self.quota_user.local_session_dir, file_name)
        file_size = 4096
        max_bytes = 10000
        lib.make_file(
            file_path,
            file_size,
            contents="arbitrary",
        )

        try:
            self.admin.assert_icommand(
                ["iadmin", "set_grid_configuration", "logical_quotas", "enabled", "1"]
            )

            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "bytes",
                    str(max_bytes),
                ]
            )
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        "<unset>",
                        str(-max_bytes),
                        "<unenforced>",
                    )
                )
                in out
            )

            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{self.quota_user.session_collection}/{file_name}",
                ]
            )
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        "<unset>",
                        str(file_size - max_bytes),
                        "<unenforced>",
                    )
                )
                in out
            )

            max_objects = 50
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "objects",
                    str(max_objects),
                ]
            )
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        str(max_objects),
                        str(file_size - max_bytes),
                        str(1 - max_objects),
                    )
                )
                in out
            )

            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{self.quota_user.session_collection}/{file_name}_2",
                ]
            )
            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{self.quota_user.session_collection}/{file_name}_3",
                ]
            )
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        str(max_objects),
                        str(file_size * 3 - max_bytes),
                        str(3 - max_objects),
                    )
                )
                in out
            )

            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{file_name}_4",
                ],
                "STDERR_SINGLELINE",
                "-186000 LOGICAL_QUOTA_EXCEEDED",
            )
            ec, _, _ = self.quota_user.assert_icommand(
                ["itouch", f"{file_name}_4"],
                "STDOUT_SINGLELINE",
                "CAT_NOT_A_DATAOBJ_AND_NOT_A_COLLECTION",
            )
            self.assertNotEqual(ec, 0)
            self.quota_user.assert_icommand(
                ["istream", "write", f"{file_name}_4"],
                "STDERR",
                ["Error: Cannot open data object"],
                input="some data",
            )

            larger_max_bytes = 100000
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "bytes",
                    str(larger_max_bytes),
                ]
            )
            fewer_max_objects = 2
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "objects",
                    str(fewer_max_objects),
                ]
            )
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(larger_max_bytes),
                        str(fewer_max_objects),
                        str(file_size * 3 - larger_max_bytes),
                        str(3 - fewer_max_objects),
                    )
                )
                in out
            )
            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{file_name}_4",
                ],
                "STDERR_SINGLELINE",
                "-186000 LOGICAL_QUOTA_EXCEEDED",
            )
            ec, _, _ = self.quota_user.assert_icommand(
                ["itouch", f"{file_name}_4"],
                "STDOUT_SINGLELINE",
                "CAT_NOT_A_DATAOBJ_AND_NOT_A_COLLECTION",
            )
            self.assertNotEqual(ec, 0)

            self.quota_user.assert_icommand(
                ["istream", "write", f"{file_name}_4"],
                "STDERR",
                ["Error: Cannot open data object"],
                input="some data",
            )

            more_max_objects = 10
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "objects",
                    str(more_max_objects),
                ]
            )
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(larger_max_bytes),
                        str(more_max_objects),
                        str(file_size * 3 - larger_max_bytes),
                        str(3 - more_max_objects),
                    )
                )
                in out
            )

            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{file_name}_4",
                ]
            )

            # Lift quota temporarily.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "0",
                    "0",
                ]
            )

            # No quota set.
            self.admin.assert_icommand(["iadmin", "list_logical_quotas"])

            object_only_limit = 4
            # Set object limit only.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "objects",
                    str(object_only_limit),
                ]
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        "<unset>",
                        str(object_only_limit),
                        "<unenforced>",
                        str(4 - object_only_limit),
                    )
                )
                in out
            )

            # Should violate object count after put.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{file_name}_5",
                ]
            )
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        "<unset>",
                        str(object_only_limit),
                        "<unenforced>",
                        str(5 - object_only_limit),
                    )
                )
                in out
            )

            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{file_name}_6",
                ],
                "STDERR_SINGLELINE",
                "-186000 LOGICAL_QUOTA_EXCEEDED",
            )

        finally:
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "0",
                    "0",
                ]
            )
            self.admin.assert_icommand(
                ["iadmin", "set_grid_configuration", "logical_quotas", "enabled", "0"]
            )

    def test_nested_logical_quota_enforcement(self):
        nesting_depth = 5
        self.quota_user.assert_icommand(
            [
                "imkdir",
                f"{self.quota_user.session_collection}/test_nested_logical_quota_coll",
            ]
        )

        def subcoll_path_generator():
            path = "test_nested_logical_quota_coll"
            count = 0
            while True:
                path = f"{path}/{count}"
                count = count + 1
                yield path

        file_name = "test_nested_logical_quota_enforcement"
        file_path = os.path.join(self.quota_user.local_session_dir, file_name)
        file_size = 10001
        # Create file that triggers quota violation by itself.
        lib.make_file(
            file_path,
            file_size,
            contents="arbitrary",
        )

        try:
            self.admin.assert_icommand(
                ["iadmin", "set_grid_configuration", "logical_quotas", "enabled", "1"]
            )

            path_gen = subcoll_path_generator()
            innermost_subcoll = ""

            max_bytes = 10000
            original_max_bytes = 10000
            # Set nested quotas.
            # Make outermost quota most restrictive.

            # Create subcollections up to nesting_depth;
            # suppose it is 5. It will create
            # test_nested_logical_quota_coll/0/1/2/3/4
            # and it will set the byte limit for the outermost
            # "0" to be the most restrictive.

            # Expected setup:
            # test_nested_logical_quota_coll
            #   0 (byte_limit: 10000)
            #     1 (byte_limit: 20000)
            #       2 (byte_limit: 40000)
            #         3 (byte_limit: 80000)
            #           4 (byte_limit: 160000)

            for i in range(0, nesting_depth):
                subcoll = next(path_gen)
                self.quota_user.assert_icommand(
                    ["imkdir", f"{self.quota_user.session_collection}/{subcoll}"]
                )
                self.admin.assert_icommand(
                    [
                        "iadmin",
                        "set_logical_quota",
                        f"{self.quota_user.session_collection}/{subcoll}",
                        "bytes",
                        str(max_bytes),
                    ]
                )
                _, out, _ = self.admin.assert_icommand(
                    ["iadmin", "list_logical_quotas"],
                    "STDOUT_SINGLELINE",
                    f"{self.quota_user.session_collection}/{subcoll}",
                )
                self.assertTrue(
                    (
                        self.llq_output_template
                        % (
                            f"{self.quota_user.session_collection}/{subcoll}",
                            str(max_bytes),
                            "<unset>",
                            str(-max_bytes),
                            "<unenforced>",
                        )
                    )
                    in out
                )
                max_bytes = max_bytes * 2
                innermost_subcoll = subcoll

            path_gen = subcoll_path_generator()
            subcoll = next(path_gen)

            # Put to innermost subcoll.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{self.quota_user.session_collection}/{innermost_subcoll}/{file_name}",
                ]
            )
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                f"{self.quota_user.session_collection}/{subcoll}",
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        f"{self.quota_user.session_collection}/{subcoll}",
                        str(original_max_bytes),
                        "<unset>",
                        str(file_size - original_max_bytes),
                        "<unenforced>",
                    )
                )
                in out
            )

            # Putting to innermost subcoll should violate the outermost subcoll's quota
            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{self.quota_user.session_collection}/{innermost_subcoll}/{file_name}_2",
                ],
                "STDERR_SINGLELINE",
                "-186000 LOGICAL_QUOTA_EXCEEDED",
            )

            # Lift byte restriction on outermost subcoll.
            # Add 2-object quota on outermost.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    f"{self.quota_user.session_collection}/{subcoll}",
                    "0",
                    "2",
                ]
            )
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # Should succeed now
            # Should violate byte quota one level deep after recalculation
            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{self.quota_user.session_collection}/{innermost_subcoll}/{file_name}_2",
                ]
            )
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{self.quota_user.session_collection}/{innermost_subcoll}/{file_name}_3",
                ],
                "STDERR_SINGLELINE",
                "-186000 LOGICAL_QUOTA_EXCEEDED",
            )

            # Lift byte restriction on one-level-deep subcoll
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    f"{self.quota_user.session_collection}/{next(path_gen)}",
                    "bytes",
                    "0",
                ]
            )
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # Should succeed now
            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{self.quota_user.session_collection}/{innermost_subcoll}/{file_name}_3",
                ]
            )
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # Should now violate object quota on outermost collection
            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{self.quota_user.session_collection}/{innermost_subcoll}/{file_name}_4",
                ],
                "STDERR_SINGLELINE",
                "-186000 LOGICAL_QUOTA_EXCEEDED",
            )

        finally:
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "0",
                    "0",
                ]
            )
            self.admin.assert_icommand(
                ["iadmin", "set_grid_configuration", "logical_quotas", "enabled", "0"]
            )

    @unittest.skipIf(
        IrodsConfig().default_rule_engine_plugin == "irods_rule_engine_plugin-python",
        "Only implemented with NREP",
    )
    def test_logical_quota_msi(self):
        file_name = "test_logical_quota_msi"
        file_path = os.path.join(self.quota_user.local_session_dir, file_name)
        file_size = 4096

        lib.make_file(
            file_path,
            file_size,
            contents="arbitrary",
        )
        self.admin.assert_icommand(
            ["iadmin", "set_grid_configuration", "logical_quotas", "enabled", "1"]
        )

        max_bytes = 4000
        try:
            rep_instance_name = "irods_rule_engine_plugin-irods_rule_language-instance"

            self.admin.assert_icommand(
                [
                    "irule",
                    "-r",
                    rep_instance_name,
                    f'msi_set_logical_quota("{self.quota_user.session_collection}", "bytes", "{max_bytes}")',
                    "null",
                    "null",
                ]
            )
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        "<unset>",
                        str(-max_bytes),
                        "<unenforced>",
                    )
                )
                in out
            )

            other_max_bytes = 3000
            max_objects = 10
            # Check that the alternate invocation works too.
            self.admin.assert_icommand(
                [
                    "irule",
                    "-r",
                    rep_instance_name,
                    f'msi_set_logical_quota("{self.quota_user.session_collection}", "{other_max_bytes}", "{max_objects}")',
                    "null",
                    "null",
                ]
            )
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            # Bytes over still at -4000 since no recalculation done yet
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(other_max_bytes),
                        str(max_objects),
                        str(-max_bytes),
                        "0",
                    )
                )
                in out
            )
            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{self.quota_user.session_collection}/{file_name}",
                ]
            )
            self.admin.assert_icommand(
                [
                    "irule",
                    "-r",
                    rep_instance_name,
                    f"msi_calc_logical_usage",
                    "null",
                    "null",
                ]
            )
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(other_max_bytes),
                        str(max_objects),
                        str(file_size - other_max_bytes),
                        str(1 - max_objects),
                    )
                )
                in out
            )
            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{file_name}_2",
                ],
                "STDERR_SINGLELINE",
                "-186000 LOGICAL_QUOTA_EXCEEDED",
            )

        finally:
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "0",
                    "0",
                ]
            )
            self.admin.assert_icommand(
                ["iadmin", "set_grid_configuration", "logical_quotas", "enabled", "0"]
            )

    def test_logical_quota_grid_configuration_settings(self):
        file_name = "test_logical_quota_grid"
        file_path = os.path.join(self.quota_user.local_session_dir, file_name)
        file_size = 4096

        lib.make_file(
            file_path,
            file_size,
            contents="arbitrary",
        )

        try:
            # No enforcement when disabled.
            self.admin.assert_icommand(
                ["iadmin", "set_grid_configuration", "logical_quotas", "enabled", "0"]
            )

            max_bytes = 4000
            # Byte quota is low enough to be violated in one upload.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "bytes",
                    str(max_bytes),
                ]
            )
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        "<unset>",
                        str(-max_bytes),
                        "<unenforced>",
                    )
                )
                in out
            )

            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{self.quota_user.session_collection}/{file_name}",
                ]
            )
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # Check to ensure numbers are correct.
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        "<unset>",
                        str(file_size - max_bytes),
                        "<unenforced>",
                    )
                )
                in out
            )

            # No failure, because no enforcement.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{self.quota_user.session_collection}/{file_name}_2",
                ]
            )
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # No enforcement when set to strange value.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_grid_configuration",
                    "logical_quotas",
                    "enabled",
                    "potato",
                ]
            )

            # No failure, because no enforcement.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{self.quota_user.session_collection}/{file_name}_3",
                ]
            )
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # Enforcement when explicitly enabled.
            self.admin.assert_icommand(
                ["iadmin", "set_grid_configuration", "logical_quotas", "enabled", "1"]
            )

            # Should fail due to enabled enforcement.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{file_name}_4",
                ],
                "STDERR_SINGLELINE",
                "-186000 LOGICAL_QUOTA_EXCEEDED",
            )

            # No enforcement when value is completely missing.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "asq",
                    "DELETE FROM R_GRID_CONFIGURATION WHERE namespace='logical_quotas' AND option_name='enabled'",
                    "rmlqgridopt",
                ]
            )
            self.admin.assert_icommand(
                ["iquest", "--sql", "rmlqgridopt"],
                "STDOUT_SINGLELINE",
                "CAT_NO_ROWS_FOUND",
            )

            # No failure, because no enforcement.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{self.quota_user.session_collection}/{file_name}_4",
                ]
            )

        finally:
            # Run and don't assert these-- if failure happens midway
            # we can't guarantee what state the catalog is in.
            self.admin.run_icommand(
                [
                    "iadmin",
                    "asq",
                    "INSERT INTO R_GRID_CONFIGURATION VALUES('logical_quotas','enabled','0')",
                    "addlqgridopt",
                ]
            )
            self.admin.run_icommand(["iquest", "--sql", "addlqgridopt"])
            self.admin.run_icommand(["iadmin", "rsq", "rmlqgridopt"])
            self.admin.run_icommand(["iadmin", "rsq", "addlqgridopt"])

            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "0",
                    "0",
                ]
            )

            # This will fail if the previous "restore the catalog" cleanup failed.
            # We don't want to progress with the catalog in a bad state.
            self.admin.assert_icommand(
                ["iadmin", "set_grid_configuration", "logical_quotas", "enabled", "0"]
            )

    def test_logical_quota_with_replicas(self):
        dataobj_name = "test_logical_quota_replica"
        small_file_name = "test_logical_quota_smallfile"
        small_file_path = os.path.join(
            self.quota_user.local_session_dir, small_file_name
        )
        small_file_size = 2048
        big_file_name = "test_logical_quota_bigfile"
        big_file_path = os.path.join(self.quota_user.local_session_dir, big_file_name)
        # If modifying, ensure value is >small_file_size
        big_file_size = 4096
        resc_name = "ufs0_logical_quota_replica"
        other_resc_name = "ufs1_logical_quota_replica"

        lib.make_file(
            small_file_path,
            small_file_size,
            contents="arbitrary",
        )
        lib.make_file(
            big_file_path,
            big_file_size,
            contents="arbitrary",
        )

        try:
            lib.create_ufs_resource(self.admin, resc_name)
            lib.create_ufs_resource(self.admin, other_resc_name)

            # Enable enforcement.
            self.admin.assert_icommand(
                ["iadmin", "set_grid_configuration", "logical_quotas", "enabled", "1"]
            )

            max_bytes = 10000
            max_objects = 5
            # Set to 10000 bytes and 5 objects.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    str(max_bytes),
                    str(max_objects),
                ]
            )
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        str(max_objects),
                        str(-max_bytes),
                        str(-max_objects),
                    )
                )
                in out
            )

            # Replica 0 is now large file.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}",
                ]
            )
            # Replica 1 is now large file.
            self.quota_user.assert_icommand(
                [
                    "irepl",
                    "-R",
                    resc_name,
                    f"{self.quota_user.session_collection}/{dataobj_name}",
                ]
            )

            self.assertEqual(
                str(1),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}",
                    self.quota_user.default_resource,
                ),
            )
            self.assertEqual(
                str(1),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}",
                    resc_name,
                ),
            )

            # Ensure only one copy of the data object is counted, even
            # when there are two good replicas.
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )

            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        str(max_objects),
                        str(big_file_size - max_bytes),
                        str(1 - max_objects),
                    )
                )
                in out
            )

            # Replica 0 is now small file and good.
            # Replica 1 is stale.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    "-f",
                    small_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}",
                ]
            )
            self.assertEqual(
                str(1),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}",
                    self.quota_user.default_resource,
                ),
            )
            self.assertEqual(
                str(0),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}",
                    resc_name,
                ),
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # Check: Replica 1 is stale and replica 0 is good,
            # so logical quotas should calculate using replica 0.
            # Replica 0 has smaller size, so verify smaller size is counted against
            # the byte limit. Additionally, verify the data object
            # counts against the object limit only once.
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        str(max_objects),
                        str(small_file_size - max_bytes),
                        str(1 - max_objects),
                    )
                )
                in out
            )

            # New data object: Replica 0 is now small file.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    small_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                ]
            )

            # Replica 1 is now small file.
            self.quota_user.assert_icommand(
                [
                    "irepl",
                    "-R",
                    resc_name,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                ]
            )

            # Replica 2 is now small file.
            self.quota_user.assert_icommand(
                [
                    "irepl",
                    "-R",
                    other_resc_name,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                ]
            )

            # Replica 0 is now big file.
            # Replicas 1, 2 are now stale.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    "-f",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                ]
            )

            self.assertEqual(
                str(0),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                    resc_name,
                ),
            )
            self.assertEqual(
                str(0),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                    other_resc_name,
                ),
            )

            # Force-stale replica 0.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "0",
                ]
            )

            self.assertEqual(
                str(0),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                    self.quota_user.default_resource,
                ),
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # Check: All replicas are stale, so logical quotas
            # should calculate using the largest replica (i.e. 0)
            # for byte_limit calculations.
            # Object should still count only once against object_limit.
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        str(max_objects),
                        str(small_file_size + big_file_size - max_bytes),
                        str(2 - max_objects),
                    )
                )
                in out
            )

            # New data object: Replica 0 is big file.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_2",
                ]
            )
            # Set to 3 (read-locked).
            # Read-locked should be ignored for byte calculations.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_2",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "3",
                ]
            )

            self.assertEqual(
                str(3),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_2",
                    self.quota_user.default_resource,
                ),
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # Read-locked should not count against byte_limit.
            # It will count against object_limit, however.
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        str(max_objects),
                        str(small_file_size + big_file_size - max_bytes),
                        str(3 - max_objects),
                    )
                )
                in out
            )

            # New data object: Replica 0 is big file.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_3",
                ]
            )
            # Set to 4 (write-locked).
            # Write-locked should be counted for byte calculations.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_3",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "4",
                ]
            )

            self.assertEqual(
                str(4),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_3",
                    self.quota_user.default_resource,
                ),
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # Write-locked should count against byte_limit.
            # It will also count against object_limit.
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        str(max_objects),
                        str(small_file_size + big_file_size * 2 - max_bytes),
                        str(4 - max_objects),
                    )
                )
                in out
            )

            # Byte limit is now in violation, so should fail.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_4",
                ],
                "STDERR_SINGLELINE",
                "-186000 LOGICAL_QUOTA_EXCEEDED",
            )

            # Return to the "3 stale replicas" case.
            # Set a small-file replica to "good."
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                    "replica_number",
                    "1",
                    "DATA_REPL_STATUS",
                    "1",
                ]
            )

            self.assertEqual(
                str(1),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                    resc_name,
                ),
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # Since the "3 stale replicas" now has a good replica
            # in the form of a small file, that size now counts
            # instead of the large file size.
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        str(max_objects),
                        str(small_file_size * 2 + big_file_size - max_bytes),
                        str(4 - max_objects),
                    )
                )
                in out
            )

            # New data object: Replica 0 is big file.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_4",
                ]
            )
            # Set to 2 (intermediate).
            # Intermediate should be ignored for byte calculations,
            # and counted for object calculations.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_4",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "2",
                ]
            )

            self.assertEqual(
                str(2),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_4",
                    self.quota_user.default_resource,
                ),
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        str(max_objects),
                        str(small_file_size * 2 + big_file_size - max_bytes),
                        str(5 - max_objects),
                    )
                )
                in out
            )

            # New data object: Replica 0 is big file.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_6",
                ]
            )

            # Set to 7 (???).
            # Unused/unknown values should be ignored for byte calculations,
            # and counted for object calculations.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_6",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "7",
                ]
            )

            self.assertEqual(
                str(7),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_6",
                    self.quota_user.default_resource,
                ),
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        str(max_objects),
                        str(small_file_size * 2 + big_file_size - max_bytes),
                        str(6 - max_objects),
                    )
                )
                in out
            )

            # Object limit is violated, so should fail.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_7",
                ],
                "STDERR_SINGLELINE",
                "-186000 LOGICAL_QUOTA_EXCEEDED",
            )

        finally:
            # Reset all the objects set to locked/intermediate;
            # deletes will fail otherwise.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_4",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "1",
                ]
            )
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_3",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "1",
                ]
            )
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_2",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "1",
                ]
            )

            # Cleanup.
            self.quota_user.assert_icommand(
                ["irm", "-f", f"{self.quota_user.session_collection}/{dataobj_name}"]
            )
            self.quota_user.assert_icommand(
                ["irm", "-f", f"{self.quota_user.session_collection}/{dataobj_name}_1"]
            )
            self.admin.run_icommand(["iadmin", "rmresc", resc_name])
            self.admin.run_icommand(["iadmin", "rmresc", other_resc_name])

            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "0",
                    "0",
                ]
            )
            self.admin.assert_icommand(
                ["iadmin", "set_grid_configuration", "logical_quotas", "enabled", "0"]
            )

    def test_logical_quota_with_register(self):
        dataobj_name = "test_logical_quota_register"
        file_name = "test_logical_quota_register_file"
        file_path = os.path.join(self.quota_user.local_session_dir, file_name)
        file_size = 2048
        resc_name = 'test_logical_quota_register_resc'
        resc_hostname = None
        if test.settings.RUN_IN_TOPOLOGY:
            if test.settings.TOPOLOGY_FROM_RESOURCE_SERVER:
                resc_hostname = test.settings.HOSTNAME_1
            else:
                 resc_hostname = test.settings.ICAT_HOSTNAME
        lib.create_ufs_resource(self.admin, resc_name, resc_hostname)
        empty_file_name = "test_logical_quota_register_empty"
        empty_file_path = os.path.join(
            self.quota_user.local_session_dir, empty_file_name
        )

        lib.make_file(
            file_path,
            file_size,
            contents="arbitrary",
        )
        lib.make_file(
            empty_file_path,
            0,
            contents="arbitrary",
        )

        try:
            # Enable enforcement.
            self.admin.assert_icommand(
                ["iadmin", "set_grid_configuration", "logical_quotas", "enabled", "1"]
            )

            max_bytes = 4000
            max_objects = 3
            # Set to 4000 bytes and 3 objects.
            # Use admin session_collection here, since registration
            # is admin-only by default.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.admin.session_collection,
                    str(max_bytes),
                    str(max_objects),
                ]
            )
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.admin.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.admin.session_collection,
                        str(max_bytes),
                        str(max_objects),
                        str(-max_bytes),
                        str(-max_objects),
                    )
                )
                in out
            )
            # Should succeed.
            self.admin.assert_icommand(
                [
                    "ireg",
                    "-R",
                    resc_name,
                    file_path,
                    f"{self.admin.session_collection}/{file_name}",
                ]
            )
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # Should succeed again.
            self.admin.assert_icommand(
                [
                    "ireg",
                    file_path,
                    "-R",
                    resc_name,
                    f"{self.admin.session_collection}/{file_name}_1",
                ]
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.admin.session_collection,
            )

            # Check math.
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.admin.session_collection,
                        str(max_bytes),
                        str(max_objects),
                        str(file_size * 2 - max_bytes),
                        2 - max_objects,
                    )
                )
                in out
            )

            # Byte limit is violated, so should fail.
            self.admin.assert_icommand(
                [
                    "ireg",
                    "-R",
                    resc_name,
                    file_path,
                    f"{self.admin.session_collection}/{file_name}_2",
                ],
                "STDERR_SINGLELINE",
                "-186000 LOGICAL_QUOTA_EXCEEDED",
            )

            # Object limit is not in violation, so an empty file
            # should be OK.
            self.admin.assert_icommand(
                [
                    "ireg",
                    "-R",
                    resc_name,
                    empty_file_path,
                    f"{self.admin.session_collection}/{empty_file_name}",
                ]
            )
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # Should still succeed.
            self.admin.assert_icommand(
                [
                    "ireg",
                    "-R",
                    resc_name,
                    empty_file_path,
                    f"{self.admin.session_collection}/{empty_file_name}_1",
                ]
            )
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.admin.session_collection,
            )

            # Check math. Object count should be in violation now.
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.admin.session_collection,
                        str(max_bytes),
                        str(max_objects),
                        str(file_size * 2 - max_bytes),
                        str(4 - max_objects),
                    )
                )
                in out
            )

            # Object limit is violated, so should fail.
            self.admin.assert_icommand(
                [
                    "ireg",
                    "-R",
                    resc_name,
                    file_path,
                    f"{self.admin.session_collection}/{empty_file_name}_2",
                ],
                "STDERR_SINGLELINE",
                "-186000 LOGICAL_QUOTA_EXCEEDED",
            )

        finally:
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.admin.session_collection,
                    "0",
                    "0",
                ]
            )
            self.admin.assert_icommand(
                ["iadmin", "set_grid_configuration", "logical_quotas", "enabled", "0"]
            )

    def test_logical_quotas_enforces_bytes_only_quota_limit(self):
        dataobj_name = "test_logical_quota_replica_bytesonly"
        small_file_name = "test_logical_quota_smallfile_bytesonly"
        small_file_path = os.path.join(
            self.quota_user.local_session_dir, small_file_name
        )
        small_file_size = 2048
        big_file_name = "test_logical_quota_bigfile_bytesonly"
        big_file_path = os.path.join(self.quota_user.local_session_dir, big_file_name)
        # If modifying, ensure value is >small_file_size
        big_file_size = 4096
        resc_name = "ufs0_logical_quota_replica_bytesonly"
        other_resc_name = "ufs1_logical_quota_replica_bytesonly"

        lib.make_file(
            small_file_path,
            small_file_size,
            contents="arbitrary",
        )
        lib.make_file(
            big_file_path,
            big_file_size,
            contents="arbitrary",
        )

        try:
            lib.create_ufs_resource(self.admin, resc_name)
            lib.create_ufs_resource(self.admin, other_resc_name)

            # Enable enforcement.
            self.admin.assert_icommand(
                ["iadmin", "set_grid_configuration", "logical_quotas", "enabled", "1"]
            )

            max_bytes = 10000
            # Set to 10000 bytes.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "bytes",
                    str(max_bytes),
                ]
            )
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        "<unset>",
                        str(-max_bytes),
                        "<unenforced>",
                    )
                )
                in out
            )

            # Replica 0 is now large file.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}",
                ]
            )
            # Replica 1 is now large file.
            self.quota_user.assert_icommand(
                [
                    "irepl",
                    "-R",
                    resc_name,
                    f"{self.quota_user.session_collection}/{dataobj_name}",
                ]
            )

            self.assertEqual(
                str(1),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}",
                    self.quota_user.default_resource,
                ),
            )
            self.assertEqual(
                str(1),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}",
                    resc_name,
                ),
            )

            # Ensure only one copy of the data object is counted, even
            # when there are two good replicas.
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )

            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        "<unset>",
                        str(big_file_size - max_bytes),
                        "<unenforced>",
                    )
                )
                in out
            )

            # Replica 0 is now small file and good.
            # Replica 1 is stale.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    "-f",
                    small_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}",
                ]
            )
            self.assertEqual(
                str(1),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}",
                    self.quota_user.default_resource,
                ),
            )
            self.assertEqual(
                str(0),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}",
                    resc_name,
                ),
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # Check: Replica 1 is stale and replica 0 is good,
            # so logical quotas should calculate using replica 0.
            # Replica 0 has smaller size, so verify smaller size is counted against
            # the byte limit. Additionally, verify the data object
            # counts against the object limit only once.
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        "<unset>",
                        str(small_file_size - max_bytes),
                        "<unenforced>",
                    )
                )
                in out
            )

            # New data object: Replica 0 is now small file.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    small_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                ]
            )

            # Replica 1 is now small file.
            self.quota_user.assert_icommand(
                [
                    "irepl",
                    "-R",
                    resc_name,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                ]
            )

            # Replica 2 is now small file.
            self.quota_user.assert_icommand(
                [
                    "irepl",
                    "-R",
                    other_resc_name,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                ]
            )

            # Replica 0 is now big file.
            # Replicas 1, 2 are now stale.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    "-f",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                ]
            )

            self.assertEqual(
                str(0),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                    resc_name,
                ),
            )
            self.assertEqual(
                str(0),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                    other_resc_name,
                ),
            )

            # Force-stale replica 0.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "0",
                ]
            )

            self.assertEqual(
                str(0),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                    self.quota_user.default_resource,
                ),
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # Check: All replicas are stale, so logical quotas
            # should calculate using the largest replica (i.e. 0)
            # for byte_limit calculations.
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        "<unset>",
                        str(small_file_size + big_file_size - max_bytes),
                        "<unenforced>",
                    )
                )
                in out
            )

            # New data object: Replica 0 is big file.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_2",
                ]
            )
            # Set to 3 (read-locked).
            # Read-locked should be ignored for byte calculations.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_2",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "3",
                ]
            )

            self.assertEqual(
                str(3),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_2",
                    self.quota_user.default_resource,
                ),
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # Read-locked should not count against byte_limit.
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        "<unset>",
                        str(small_file_size + big_file_size - max_bytes),
                        "<unenforced>",
                    )
                )
                in out
            )

            # New data object: Replica 0 is big file.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_3",
                ]
            )
            # Set to 4 (write-locked).
            # Write-locked should be counted for byte calculations.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_3",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "4",
                ]
            )

            self.assertEqual(
                str(4),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_3",
                    self.quota_user.default_resource,
                ),
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # Write-locked should count against byte_limit.
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        "<unset>",
                        str(small_file_size + big_file_size * 2 - max_bytes),
                        "<unenforced>",
                    )
                )
                in out
            )

            # Byte limit is now in violation, so should fail.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_4",
                ],
                "STDERR_SINGLELINE",
                "-186000 LOGICAL_QUOTA_EXCEEDED",
            )

            # Return to the "3 stale replicas" case.
            # Set a small-file replica to "good."
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                    "replica_number",
                    "1",
                    "DATA_REPL_STATUS",
                    "1",
                ]
            )

            self.assertEqual(
                str(1),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                    resc_name,
                ),
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # Since the "3 stale replicas" now has a good replica
            # in the form of a small file, that size now counts
            # instead of the large file size.
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        "<unset>",
                        str(small_file_size * 2 + big_file_size - max_bytes),
                        "<unenforced>",
                    )
                )
                in out
            )

            # New data object: Replica 0 is big file.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_4",
                ]
            )
            # Set to 2 (intermediate).
            # Intermediate should be ignored for byte calculations.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_4",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "2",
                ]
            )

            self.assertEqual(
                str(2),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_4",
                    self.quota_user.default_resource,
                ),
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        "<unset>",
                        str(small_file_size * 2 + big_file_size - max_bytes),
                        "<unenforced>",
                    )
                )
                in out
            )

            # New data object: Replica 0 is big file.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_6",
                ]
            )

            # Set to 7 (???).
            # Unused/unknown values should be ignored for byte calculations.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_6",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "7",
                ]
            )

            self.assertEqual(
                str(7),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_6",
                    self.quota_user.default_resource,
                ),
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        "<unset>",
                        str(small_file_size * 2 + big_file_size - max_bytes),
                        "<unenforced>",
                    )
                )
                in out
            )

            # No quota is violated, so should succeed..
            self.quota_user.assert_icommand(
                [
                    "iput",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_7",
                ]
            )

        finally:
            # Reset all the objects set to locked/intermediate;
            # deletes will fail otherwise.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_4",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "1",
                ]
            )
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_3",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "1",
                ]
            )
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_2",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "1",
                ]
            )

            # Cleanup.
            self.quota_user.assert_icommand(
                ["irm", "-f", f"{self.quota_user.session_collection}/{dataobj_name}"]
            )
            self.quota_user.assert_icommand(
                ["irm", "-f", f"{self.quota_user.session_collection}/{dataobj_name}_1"]
            )
            self.admin.run_icommand(["iadmin", "rmresc", resc_name])
            self.admin.run_icommand(["iadmin", "rmresc", other_resc_name])

            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "0",
                    "0",
                ]
            )
            self.admin.assert_icommand(
                ["iadmin", "set_grid_configuration", "logical_quotas", "enabled", "0"]
            )

    def test_logical_quotas_enforces_objects_only_quota_limit(self):
        dataobj_name = "test_logical_quota_replica_onlyobjects"
        small_file_name = "test_logical_quota_smallfile_onlyobjects"
        small_file_path = os.path.join(
            self.quota_user.local_session_dir, small_file_name
        )
        small_file_size = 2048
        big_file_name = "test_logical_quota_bigfile_onlyobjects"
        big_file_path = os.path.join(self.quota_user.local_session_dir, big_file_name)
        big_file_size = 4096
        resc_name = "ufs0_logical_quota_replica_onlyobjects"
        other_resc_name = "ufs1_logical_quota_replica_onlyobjects"

        lib.make_file(
            small_file_path,
            small_file_size,
            contents="arbitrary",
        )
        lib.make_file(
            big_file_path,
            big_file_size,
            contents="arbitrary",
        )

        try:
            lib.create_ufs_resource(self.admin, resc_name)
            lib.create_ufs_resource(self.admin, other_resc_name)

            # Enable enforcement.
            self.admin.assert_icommand(
                ["iadmin", "set_grid_configuration", "logical_quotas", "enabled", "1"]
            )

            max_objects = 5
            # Set to 10000 bytes and 5 objects.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "objects",
                    str(max_objects),
                ]
            )
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        "<unset>",
                        str(max_objects),
                        "<unenforced>",
                        str(-max_objects),
                    )
                )
                in out
            )

            # Replica 0 is now large file.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}",
                ]
            )
            # Replica 1 is now large file.
            self.quota_user.assert_icommand(
                [
                    "irepl",
                    "-R",
                    resc_name,
                    f"{self.quota_user.session_collection}/{dataobj_name}",
                ]
            )

            self.assertEqual(
                str(1),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}",
                    self.quota_user.default_resource,
                ),
            )
            self.assertEqual(
                str(1),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}",
                    resc_name,
                ),
            )

            # Ensure only one copy of the data object is counted, even
            # when there are two good replicas.
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )

            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        "<unset>",
                        str(max_objects),
                        "<unenforced>",
                        str(1 - max_objects),
                    )
                )
                in out
            )

            # Replica 0 is now small file and good.
            # Replica 1 is stale.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    "-f",
                    small_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}",
                ]
            )
            self.assertEqual(
                str(1),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}",
                    self.quota_user.default_resource,
                ),
            )
            self.assertEqual(
                str(0),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}",
                    resc_name,
                ),
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # Check: Replica 1 is stale and replica 0 is good.
            # Verify the data object
            # counts against the object limit exactly once.
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        "<unset>",
                        str(max_objects),
                        "<unenforced>",
                        str(1 - max_objects),
                    )
                )
                in out
            )

            # New data object: Replica 0 is now small file.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    small_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                ]
            )

            # Replica 1 is now small file.
            self.quota_user.assert_icommand(
                [
                    "irepl",
                    "-R",
                    resc_name,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                ]
            )

            # Replica 2 is now small file.
            self.quota_user.assert_icommand(
                [
                    "irepl",
                    "-R",
                    other_resc_name,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                ]
            )

            # Replica 0 is now big file.
            # Replicas 1, 2 are now stale.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    "-f",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                ]
            )

            self.assertEqual(
                str(0),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                    resc_name,
                ),
            )
            self.assertEqual(
                str(0),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                    other_resc_name,
                ),
            )

            # Force-stale replica 0.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "0",
                ]
            )

            self.assertEqual(
                str(0),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                    self.quota_user.default_resource,
                ),
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # Check: All replicas are stale, but object should 
            # still count exactly once against object_limit.
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        "<unset>",
                        str(max_objects),
                        "<unenforced>",
                        str(2 - max_objects),
                    )
                )
                in out
            )

            # New data object: Replica 0 is big file.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_2",
                ]
            )
            # Set to 3 (read-locked).
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_2",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "3",
                ]
            )

            self.assertEqual(
                str(3),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_2",
                    self.quota_user.default_resource,
                ),
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # Read-locked should count against object_limit.
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        "<unset>",
                        str(max_objects),
                        "<unenforced>",
                        str(3 - max_objects),
                    )
                )
                in out
            )

            # New data object: Replica 0 is big file.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_3",
                ]
            )
            # Set to 4 (write-locked).
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_3",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "4",
                ]
            )

            self.assertEqual(
                str(4),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_3",
                    self.quota_user.default_resource,
                ),
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # Write-locked should count against object_limit.
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        "<unset>",
                        str(max_objects),
                        "<unenforced>",
                        str(4 - max_objects),
                    )
                )
                in out
            )

            # Return to the "3 stale replicas" case.
            # Set a small-file replica to "good."
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                    "replica_number",
                    "1",
                    "DATA_REPL_STATUS",
                    "1",
                ]
            )

            self.assertEqual(
                str(1),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_1",
                    resc_name,
                ),
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            # Since the "3 stale replicas" now has a good replica
            # in the form of a small file, that size now counts
            # instead of the large file size.
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        "<unset>",
                        str(max_objects),
                        "<unenforced>",
                        str(4 - max_objects),
                    )
                )
                in out
            )

            # New data object: Replica 0 is big file.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_4",
                ]
            )
            # Set to 2 (intermediate).
            # Intermediate should be ignored for byte calculations,
            # and counted for object calculations.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_4",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "2",
                ]
            )

            self.assertEqual(
                str(2),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_4",
                    self.quota_user.default_resource,
                ),
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        "<unset>",
                        str(max_objects),
                        "<unenforced>",
                        str(5 - max_objects),
                    )
                )
                in out
            )

            # New data object: Replica 0 is big file.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_6",
                ]
            )

            # Set to 7 (???).
            # Unused/unknown values should be ignored for byte calculations,
            # and counted for object calculations.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_6",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "7",
                ]
            )

            self.assertEqual(
                str(7),
                lib.get_replica_status_for_resource(
                    self.quota_user,
                    f"{self.quota_user.session_collection}/{dataobj_name}_6",
                    self.quota_user.default_resource,
                ),
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        "<unset>",
                        str(max_objects),
                        "<unenforced>",
                        str(6 - max_objects),
                    )
                )
                in out
            )

            # Object limit is violated, so should fail.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    big_file_path,
                    f"{self.quota_user.session_collection}/{dataobj_name}_7",
                ],
                "STDERR_SINGLELINE",
                "-186000 LOGICAL_QUOTA_EXCEEDED",
            )

        finally:
            # Reset all the objects set to locked/intermediate;
            # deletes will fail otherwise.
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_4",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "1",
                ]
            )
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_3",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "1",
                ]
            )
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "modrepl",
                    "logical_path",
                    f"{self.quota_user.session_collection}/{dataobj_name}_2",
                    "replica_number",
                    "0",
                    "DATA_REPL_STATUS",
                    "1",
                ]
            )

            # Cleanup.
            self.quota_user.assert_icommand(
                ["irm", "-f", f"{self.quota_user.session_collection}/{dataobj_name}"]
            )
            self.quota_user.assert_icommand(
                ["irm", "-f", f"{self.quota_user.session_collection}/{dataobj_name}_1"]
            )
            self.admin.run_icommand(["iadmin", "rmresc", resc_name])
            self.admin.run_icommand(["iadmin", "rmresc", other_resc_name])

            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "0",
                    "0",
                ]
            )
            self.admin.assert_icommand(
                ["iadmin", "set_grid_configuration", "logical_quotas", "enabled", "0"]
            )

    def test_logical_quotas_counters_properly_decrement(self):
        file_name = "test_logical_quota_decrement_counters"
        file_path = os.path.join(self.quota_user.local_session_dir, file_name)
        file_size = 4096
        max_bytes = 10000
        max_objects = 10
        lib.make_file(
            file_path,
            file_size,
            contents="arbitrary",
        )

        try:
            self.admin.assert_icommand(
                ["iadmin", "set_grid_configuration", "logical_quotas", "enabled", "1"]
            )
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    str(max_bytes),
                    str(max_objects),
                ]
            )
            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        str(max_objects),
                        str(-max_bytes),
                        str(-max_objects),
                    )
                )
                in out
            )

            # Repeatedly put data objects until
            # quota is violated.
            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{self.quota_user.session_collection}/{file_name}",
                ]
            )
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        str(max_objects),
                        str(file_size-max_bytes),
                        str(1-max_objects),
                    )
                )
                in out
            )

            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{self.quota_user.session_collection}/{file_name}_1",
                ]
            )
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        str(max_objects),
                        str(2*file_size-max_bytes),
                        str(2-max_objects),
                    )
                )
                in out
            )

            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{self.quota_user.session_collection}/{file_name}_2",
                ]
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        str(max_objects),
                        str(3*file_size-max_bytes),
                        str(3-max_objects),
                    )
                )
                in out
            )

            self.quota_user.assert_icommand(
                [
                    "iput",
                    file_path,
                    f"{self.quota_user.session_collection}/{file_name}_3",
                ],
                "STDERR_SINGLELINE",
                "-186000 LOGICAL_QUOTA_EXCEEDED",
            )

            # Repeatedly delete data objects
            # and check calculations.
            self.quota_user.assert_icommand(
                [
                    "irm",
                    "-f",
                    f"{self.quota_user.session_collection}/{file_name}",
                ]
            )
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        str(max_objects),
                        str(2*file_size-max_bytes),
                        str(2-max_objects),
                    )
                )
                in out
            )

            self.quota_user.assert_icommand(
                [
                    "irm",
                    "-f",
                    f"{self.quota_user.session_collection}/{file_name}_1",
                ]
            )
            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        str(max_objects),
                        str(file_size-max_bytes),
                        str(1-max_objects),
                    )
                )
                in out
            )

            self.quota_user.assert_icommand(
                [
                    "irm",
                    "-f",
                    file_path,
                    f"{self.quota_user.session_collection}/{file_name}_2",
                ]
            )

            self.admin.assert_icommand(["iadmin", "calculate_logical_usage"])

            _, out, _ = self.admin.assert_icommand(
                ["iadmin", "list_logical_quotas"],
                "STDOUT_SINGLELINE",
                self.quota_user.session_collection,
            )
            self.assertTrue(
                (
                    self.llq_output_template
                    % (
                        self.quota_user.session_collection,
                        str(max_bytes),
                        str(max_objects),
                        str(-max_bytes),
                        str(-max_objects),
                    )
                )
                in out
            )


        finally:
            self.admin.assert_icommand(
                [
                    "iadmin",
                    "set_logical_quota",
                    self.quota_user.session_collection,
                    "0",
                    "0",
                ]
            )
            self.admin.assert_icommand(
                ["iadmin", "set_grid_configuration", "logical_quotas", "enabled", "0"]
            )
